/*-------------------------------------------------------------------------
 * drawElements C++ Base Library
 * -----------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *//*!
 * \file
 * \brief Thread-safe ring buffer template.
 *//*--------------------------------------------------------------------*/

#include "deThreadSafeRingBuffer.hpp"
#include "deRandom.hpp"
#include "deThread.hpp"

#include <vector>

using std::vector;

namespace de
{

namespace
{

struct Message
{
	deUint32 data;

	Message (deUint16 threadId, deUint16 payload)
		: data((threadId << 16) | payload)
	{
	}

	Message (void)
		: data(0)
	{
	}

	deUint16 getThreadId	(void) const { return (deUint16)(data >> 16);		}
	deUint16 getPayload		(void) const { return (deUint16)(data & 0xffff);	}
};

class Consumer : public Thread
{
public:
	Consumer (ThreadSafeRingBuffer<Message>& buffer, int numProducers)
		: m_buffer		(buffer)
	{
		m_lastPayload.resize(numProducers, 0);
		m_payloadSum.resize(numProducers, 0);
	}

	void run (void)
	{
		for (;;)
		{
			Message msg = m_buffer.popBack();

			deUint16 threadId = msg.getThreadId();

			if (threadId == 0xffff)
				break;

			DE_TEST_ASSERT(de::inBounds<int>(threadId, 0, (int)m_lastPayload.size()));
			DE_TEST_ASSERT((m_lastPayload[threadId] == 0 && msg.getPayload() == 0) || m_lastPayload[threadId] < msg.getPayload());

			m_lastPayload[threadId]	 = msg.getPayload();
			m_payloadSum[threadId]	+= (deUint32)msg.getPayload();
		}
	}

	deUint32 getPayloadSum (deUint16 threadId) const
	{
		return m_payloadSum[threadId];
	}

private:
	ThreadSafeRingBuffer<Message>&	m_buffer;
	vector<deUint16>				m_lastPayload;
	vector<deUint32>				m_payloadSum;
};

class Producer : public Thread
{
public:
	Producer (ThreadSafeRingBuffer<Message>& buffer, deUint16 threadId, int dataSize)
		: m_buffer		(buffer)
		, m_threadId	(threadId)
		, m_dataSize	(dataSize)
	{
	}

	void run (void)
	{
		// Yield to give main thread chance to start other producers.
		deSleep(1);

		for (int ndx = 0; ndx < m_dataSize; ndx++)
			m_buffer.pushFront(Message(m_threadId, (deUint16)ndx));
	}

private:
	ThreadSafeRingBuffer<Message>&	m_buffer;
	deUint16						m_threadId;
	int								m_dataSize;
};

} // anonymous

void ThreadSafeRingBuffer_selfTest (void)
{
	const int numIterations = 16;
	for (int iterNdx = 0; iterNdx < numIterations; iterNdx++)
	{
		Random							rnd				(iterNdx);
		int								bufSize			= rnd.getInt(1, 2048);
		int								numProducers	= rnd.getInt(1, 16);
		int								numConsumers	= rnd.getInt(1, 16);
		int								dataSize		= rnd.getInt(1000, 10000);
		ThreadSafeRingBuffer<Message>	buffer			(bufSize);
		vector<Producer*>				producers;
		vector<Consumer*>				consumers;

		for (int i = 0; i < numProducers; i++)
			producers.push_back(new Producer(buffer, (deUint16)i, dataSize));

		for (int i = 0; i < numConsumers; i++)
			consumers.push_back(new Consumer(buffer, numProducers));

		// Start consumers.
		for (vector<Consumer*>::iterator i = consumers.begin(); i != consumers.end(); i++)
			(*i)->start();

		// Start producers.
		for (vector<Producer*>::iterator i = producers.begin(); i != producers.end(); i++)
			(*i)->start();

		// Wait for producers.
		for (vector<Producer*>::iterator i = producers.begin(); i != producers.end(); i++)
			(*i)->join();

		// Write end messages for consumers.
		for (int i = 0; i < numConsumers; i++)
			buffer.pushFront(Message(0xffff, 0));

		// Wait for consumers.
		for (vector<Consumer*>::iterator i = consumers.begin(); i != consumers.end(); i++)
			(*i)->join();

		// Verify payload sums.
		deUint32 refSum = 0;
		for (int i = 0; i < dataSize; i++)
			refSum += (deUint32)(deUint16)i;

		for (int i = 0; i < numProducers; i++)
		{
			deUint32 cmpSum = 0;
			for (int j = 0; j < numConsumers; j++)
				cmpSum += consumers[j]->getPayloadSum((deUint16)i);
			DE_TEST_ASSERT(refSum == cmpSum);
		}

		// Free resources.
		for (vector<Producer*>::iterator i = producers.begin(); i != producers.end(); i++)
			delete *i;
		for (vector<Consumer*>::iterator i = consumers.begin(); i != consumers.end(); i++)
			delete *i;
	}
}

} // de
