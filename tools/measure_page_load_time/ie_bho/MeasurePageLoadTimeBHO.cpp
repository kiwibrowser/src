// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implements a Browser Helper Object (BHO) which opens a socket
// and waits to receive URLs over it. Visits those URLs, measuring
// how long it takes between the start of navigation and the
// DocumentComplete event, and returns the time in milliseconds as
// a string to the caller.

#include "stdafx.h"
#include "MeasurePageLoadTimeBHO.h"

#define MAX_URL 1024                 // size of URL buffer
#define MAX_PAGELOADTIME (4*60*1000) // assume all pages take < 4 minutes
#define PORT 42492                   // port to listen on. Also jhaas's
                                     // old MSFT employee number


// Static function to serve as thread entry point, takes a "this"
// pointer as pParam and calls the method in the object
static DWORD WINAPI ProcessPageTimeRequests(LPVOID pThis) {
  reinterpret_cast<CMeasurePageLoadTimeBHO*>(pThis)->ProcessPageTimeRequests();

  return 0;
}


STDMETHODIMP CMeasurePageLoadTimeBHO::SetSite(IUnknown* pUnkSite)
{
    if (pUnkSite != NULL)
    {
        // Cache the pointer to IWebBrowser2.
        HRESULT hr = pUnkSite->QueryInterface(IID_IWebBrowser2, (void **)&m_spWebBrowser);
        if (SUCCEEDED(hr))
        {
            // Register to sink events from DWebBrowserEvents2.
            hr = DispEventAdvise(m_spWebBrowser);
            if (SUCCEEDED(hr))
            {
                m_fAdvised = TRUE;
            }

            // Stash the interface in the global interface table
            CComGITPtr<IWebBrowser2> git(m_spWebBrowser);
            m_dwCookie = git.Detach();

            // Create the event to be signaled when navigation completes.
            // Start it in nonsignaled state, and allow it to be triggered
            // when the initial page load is done.
            m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

            // Create a thread to wait on the socket
            HANDLE hThread = CreateThread(NULL, 0, ::ProcessPageTimeRequests, this, 0, NULL);
        }
    }
    else
    {
        // Unregister event sink.
        if (m_fAdvised)
        {
            DispEventUnadvise(m_spWebBrowser);
            m_fAdvised = FALSE;
        }

        // Release cached pointers and other resources here.
        m_spWebBrowser.Release();
    }

    // Call base class implementation.
    return IObjectWithSiteImpl<CMeasurePageLoadTimeBHO>::SetSite(pUnkSite);
}


void STDMETHODCALLTYPE CMeasurePageLoadTimeBHO::OnDocumentComplete(IDispatch *pDisp, VARIANT *pvarURL)
{
    if (pDisp == m_spWebBrowser)
    {
        // Fire the event when the page is done loading
        // to unblock the other thread.
        SetEvent(m_hEvent);
    }
}


void CMeasurePageLoadTimeBHO::ProcessPageTimeRequests()
{
    CoInitialize(NULL);

    // The event will start in nonsignaled state, meaning that
    // the initial page load isn't done yet. Wait for that to
    // finish before doing anything.
    //
    // It seems to be the case that the BHO will get loaded
    // and SetSite called always before the initial page load
    // even begins, but just to be on the safe side, we won't
    // wait indefinitely.
    WaitForSingleObject(m_hEvent, MAX_PAGELOADTIME);

    // Retrieve the web browser interface from the global table
    CComGITPtr<IWebBrowser2> git(m_dwCookie);
    IWebBrowser2* browser;
    git.CopyTo(&browser);

    // Create a listening socket
    m_sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockListen == SOCKET_ERROR)
        ErrorExit();

    BOOL on = TRUE;
    if (setsockopt(m_sockListen, SOL_SOCKET, SO_REUSEADDR,
                   (const char*)&on, sizeof(on)))
        ErrorExit();

    // Bind the listening socket
    SOCKADDR_IN addrBind;

    addrBind.sin_family      = AF_INET;
    addrBind.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addrBind.sin_port        = htons(PORT);

    if (bind(m_sockListen, (sockaddr*)&addrBind, sizeof(addrBind)))
        ErrorExit();

    // Listen for incoming connections
    if (listen(m_sockListen, 1))
        ErrorExit();

    // Ensure the socket is blocking... it should be by default, but
    // it can't hurt to make sure
    unsigned long nNonblocking = 0;
    if (ioctlsocket(m_sockListen, FIONBIO, &nNonblocking))
        ErrorExit();

    m_sockTransport = 0;

    // Loop indefinitely waiting for connections
    while(1)
    {
        SOCKADDR_IN addrConnected;
        int         sConnected = sizeof(addrConnected);

        // Wait for a client to connect and send a URL
        m_sockTransport = accept(
            m_sockListen, (sockaddr*)&addrConnected, &sConnected);

        if (m_sockTransport == SOCKET_ERROR)
            ErrorExit();

        char pbBuffer[MAX_URL], strURL[MAX_URL];
        DWORD cbRead, cbWritten;

        bool fDone = false;

        // Loop until we're done with this client
        while (!fDone)
        {
            *strURL = '\0';
            bool fReceivedCR = false;

            do
            {
                // Only receive up to the first carriage return
                cbRead = recv(m_sockTransport, pbBuffer, MAX_URL-1, MSG_PEEK);

                // An error on read most likely means that the remote peer
                // closed the connection. Go back to waiting
                if (cbRead == 0)
                {
                    fDone = true;
                    break;
                }

                // Null terminate the received characters so strchr() is safe
                pbBuffer[cbRead] = '\0';

                if(char* pchFirstCR = strchr(pbBuffer, '\n'))
                {
                    cbRead = (DWORD)(pchFirstCR - pbBuffer + 1);
                    fReceivedCR = true;
                }

                // The below call will not block, since we determined with
                // MSG_PEEK that at least cbRead bytes are in the TCP receive buffer
                recv(m_sockTransport, pbBuffer, cbRead, 0);
                pbBuffer[cbRead] = '\0';

                strcat_s(strURL, sizeof(strURL), pbBuffer);
            } while (!fReceivedCR);

            // If an error occurred while reading, exit this loop
            if (fDone)
                break;

            // Strip the trailing CR and/or LF
            int i;
            for (i = (int)strlen(strURL)-1; i >= 0 && isspace(strURL[i]); i--)
            {
                strURL[i] = '\0';
            }

            if (i < 0)
            {
                // Sending a carriage return on a line by itself means that
                // the client is done making requests
                fDone = true;
            }
            else
            {
                // Send the browser to the requested URL
                CComVariant vNavFlags( navNoReadFromCache );
                CComVariant vTargetFrame("_self");
                CComVariant vPostData("");
                CComVariant vHTTPHeaders("");

                ResetEvent(m_hEvent);
                DWORD dwStartTime = GetTickCount();

                HRESULT hr = browser->Navigate(
                    CComBSTR(strURL),
                    &vNavFlags,
                    &vTargetFrame, // TargetFrameName
                    &vPostData, // PostData
                    &vHTTPHeaders // Headers
                    );

                // The main browser thread will call OnDocumentComplete() when
                // the page is done loading, which will in turn trigger
                // m_hEvent. Wait here until then; the event will reset itself
                // once this thread is released
                if (WaitForSingleObject(m_hEvent, MAX_PAGELOADTIME) == WAIT_TIMEOUT)
                {
                    sprintf_s(pbBuffer, sizeof(pbBuffer), "%s,timeout\n", strURL);

                    browser->Stop();
                }
                else
                {
                    // Format the elapsed time as a string
                    DWORD dwLoadTime = GetTickCount() - dwStartTime;
                    sprintf_s(
                        pbBuffer, sizeof(pbBuffer), "%s,%d\n", strURL, dwLoadTime);
                }

                // Send the result. Just in case the TCP buffer can't handle
                // the whole thing, send in parts if necessary
                char *chSend = pbBuffer;

                while (*chSend)
                {
                    cbWritten = send(
                        m_sockTransport, chSend, (int)strlen(chSend), 0);

                    // Error on send probably means connection reset by peer
                    if (cbWritten == 0)
                    {
                        fDone = true;
                        break;
                    }

                    chSend += cbWritten;
                }
            }
        }

        // Close the transport socket and wait for another connection
        closesocket(m_sockTransport);
        m_sockTransport = 0;
    }
}


void CMeasurePageLoadTimeBHO::ErrorExit()
{
    // Unlink from IE, close the sockets, then terminate this
    // thread
    SetSite(NULL);

    if (m_sockTransport && m_sockTransport != SOCKET_ERROR)
    {
        closesocket(m_sockTransport);
        m_sockTransport = 0;
    }

    if (m_sockListen && m_sockListen != SOCKET_ERROR)
    {
        closesocket(m_sockListen);
        m_sockListen = 0;
    }

    TerminateThread(GetCurrentThread(), -1);
}
