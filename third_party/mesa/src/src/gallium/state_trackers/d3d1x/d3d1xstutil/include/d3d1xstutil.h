/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef D3D1XSTUTIL_H_
#define D3D1XSTUTIL_H_

#ifdef _MSC_VER
#include <unordered_map>
#include <unordered_set>
#else
#include <tr1/unordered_map>
#include <tr1/unordered_set>
namespace std
{
	using namespace tr1;
}
#endif
#include <map>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <objbase.h>

#include "galliumdxgi.h"
#include <d3dcommon.h>

extern "C"
{
#include "util/u_atomic.h"
#include "pipe/p_format.h"
#include "os/os_thread.h"
}

#include <assert.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#define D3D_PRIMITIVE_TOPOLOGY_COUNT 65
extern unsigned d3d_to_pipe_prim[D3D_PRIMITIVE_TOPOLOGY_COUNT];

#define D3D_PRIMITIVE_COUNT 40
extern unsigned d3d_to_pipe_prim_type[D3D_PRIMITIVE_COUNT];

/* NOTE: this _depends_ on the vtable layout of the C++ compiler to be
 * binary compatible with Windows.
 * Furthermore some absurd vtable layout likely won't work at all, since
 * we perform some casts which are probably not safe by the C++ standard.
 *
 * In particular, the GNU/Linux/Itanium/clang ABI and Microsoft ABIs will work,
 * but others may not.
 * If in doubt, just switch to the latest version of a widely used C++ compiler.
 *
 * DESIGN of the Gallium COM implementation
 *
 * This state tracker uses somewhat unusual C++ coding patterns,
 * to implement the COM interfaces required by Direct3D.
 *
 * While it may seem complicated, the effect is that the result
 * generally behaves as intuitively as possible: in particular pointer
 * casts very rarely change the pointer value (only for secondary
 * DXGI/Gallium interfaces)
 *
 * Implementing COM is on first sight very easy: after all, it just
 * consists of a reference count, and a dynamic_cast<> equivalent.
 *
 * However, implementing objects with multiple interfaces is actually
 * quite tricky.
 * The issue is that the interface pointers can't be equal, since this
 * would place incompatible constraints on the vtable layout and thus
 * multiple inheritance (and the subobjects the C++ compiler creates
 * with it) must be correctly used.
 *
 * Furthermore, we must have a single reference count, which means
 * that a naive implementation won't work, and it's necessary to either
 * use virtual inheritance, or the "mixin inheritance" model we use.
 *
 * This solution aims to achieve the following object layout:
 * 0: pointer to vtable for primary interface
 * 1: reference count
 * ... main class
 * ... vtable pointers for secondary interfaces
 * ... implementation of subclasses assuming secondary interfaces
 *
 * This allows us to cast pointers by just reinterpreting the value in
 * almost all cases.
 *
 * To achieve this, *all* non-leaf classes must have their parent
 * or the base COM interface as a template parameter, since derived
 * classes may need to change that to support an interface derived
 * from the one implemented by the superclass.
 *
 * Note however, that you can cast without regard to the template
 * parameter, because only the vtable layout depends on it, since
 * interfaces have no data members.
 *
 * For this to work, DON'T USE VIRTUAL FUNCTIONS except to implement
 * interfaces, since the vtable layouts would otherwise be mismatched.
 * An exception are virtual functions called only from other virtual functions,
 * which is currently only used for the virtual destructor.
 *
 * The base class is GalliumComObject<IFoo>, which implements the
 * IUnknown interface, and inherits IFoo.
 *
 * To support multiple inheritance, we insert GalliumMultiComObject,
 * which redirects the secondary interfaces to the GalliumComObject
 * superclass.
 *
 * Gallium(Multi)PrivateDataComObject is like ComObject but also
 * implements the Get/SetPrivateData functions present on several
 * D3D/DXGI interfaces.
 *
 * Example class hierarchy:
 *
 * IUnknown
 * (pure interface)
 * |
 * V
 * IAnimal
 * (pure interface)
 * |
 * V
 * IDuck
 * (pure interface)
 * |
 * V
 * GalliumComObject<IDuck>
 * (non-instantiable, only implements IUnknown)
 * |
 * V
 * GalliumAnimal<IDuck>
 * (non-instantiable, only implements IAnimal)
 * |
 * V
 * GalliumDuck
 * (concrete)
 * |
 * V
 * GalliumMultiComObject<GalliumDuck, IWheeledVehicle> <- IWheeledVehicle <- IVehicle <- IUnknown (second version)
 * (non-instantiable, only implements IDuck and the IUnknown of IWheeledVehicle)
 * |
 * V
 * GalliumDuckOnWheels
 * (concrete)
 *
 * This will produce the desired layout.
 * Note that GalliumAnimal<IFoo>* is safely castable to GalliumAnimal<IBar>*
 * by reinterpreting, as long as non-interface virtual functions are not used,
 * and that you only call interface functions for the superinterface of IBar
 * that the object actually implements.
 *
 * Instead, if GalliumDuck where to inherit both from GalliumAnimal
 * and IDuck, then (IDuck*)gallium_duck and (IAnimal*)gallium_duck would
 * have different pointer values, which the "base class as template parameter"
 * trick avoids.
 *
 * The price we pay is that you MUST NOT have virtual functions other than those
 * implementing interfaces (except for leaf classes) since the position of these
 * would depend on the base interface.
 * As mentioned above, virtual functions only called from interface functions
 * are an exception, currently used only for the virtual destructor.
 * If you want virtual functions anyway , put them in a separate interface class,
 * multiply inherit from that and cast the pointer to that interface.
 *
 * You CAN however have virtual functions on any class which does not specify
 * his base as a template parameter, or where you don't need to change the
 * template base interface parameter by casting.
 *
 * --- The magic QueryInterface "delete this" trick ---
 *
 * When the reference count drops to 0, we must delete the class.
 * The problem is, that we must call the right virtual destructor (i.e. on the right class).
 * However, we would like to be able to call release() and nonatomic_release()
 * non-virtually for performance (also, the latter cannot be called virtually at all, since
 * IUnknown does not offer it).
 *
 * The naive solution would be to just add a virtual destructor and rely on it.
 * However, this doesn't work due to the fact that as described above we perform casets
 * with are unsafe regarding vtable layout.
 * In particular, consider the case where we try to delete GalliumComObject<ID3D11Texture2D>
 * with a pointer to GalliumComObject<ID3D11Resource>.
 * Since we think that this is a GalliumComObject<ID3D11Resource>, we'll look for the
 * destructor in the vtable slot immediately after the ID3D11Resource vtable, but this is
 * actually an ID3D11Texture2D function implemented by the object!
 *
 * So, we must put the destructor somewhere else.
 * We could add it as a data member, but it would be awkward and it would bloat the
 * class.
 * Thus, we use this trick: we reuse the vtable slot for QueryInterface, which is always at the
 * same position.
 * To do so, we define a special value for the first pointer argument, that triggers a
 * "delete this".
 * In addition to that, we add a virtual destructor to GalliumComObject.
 * That virtual destructor will be called by QueryInterface, and since that is a virtual
 * function, it will know the correct place for the virtual destructor.
 *
 * QueryInterface is already slow due to the need to compare several GUIDs, so the
 * additional pointer test should not be significant.
 *
 * Of course the ideal solution would be telling the C++ compiler to put the
 * destructor it in a negative vtable slot, but unfortunately GCC doesn't support that
 * yet, and this method is almost as good as that.
 */

template<typename T>
struct com_traits;

#define COM_INTERFACE(intf, base) \
template<> \
struct com_traits<intf> \
{ \
	static REFIID iid() {return IID_##intf;} \
	static inline bool is_self_or_ancestor(REFIID riid) {return riid == iid() || com_traits<base>::is_self_or_ancestor(riid);} \
};

template<>
struct com_traits<IUnknown>
{
	static REFIID iid() {return IID_IUnknown;}
	static inline bool is_self_or_ancestor(REFIID riid) {return riid == iid();}
};

#ifndef _MSC_VER
#define __uuidof(T) (com_traits<T>::iid())
#endif

struct refcnt_t
{
	uint32_t refcnt;

	refcnt_t(unsigned v = 1)
	: refcnt(v)
	{}

	unsigned add_ref()
	{
		p_atomic_inc((int32_t*)&refcnt);
		return refcnt;
	}

	unsigned release()
	{
		if(p_atomic_dec_zero((int32_t*)&refcnt))
			return 0;
		return refcnt;
	}

	void nonatomic_add_ref()
	{
		p_atomic_inc((int32_t*)&refcnt);
	}

	unsigned nonatomic_release()
	{
		if(p_atomic_dec_zero((int32_t*)&refcnt))
			return 0;
		else
			return 1;
	}
};

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)
/* this should be safe because atomic ops are full memory barriers, and thus a sequence that does:
 * ++one_refcnt;
 * --other_refcnt;
 * should never be reorderable (as seen from another CPU) to:
 * --other_refcnt
 * ++one_refcnt
 *
 * since one of the ops is atomic.
 * If this weren't the case, a CPU could incorrectly destroy an object manipulated in that way by another one.
 */
struct dual_refcnt_t
{
	union
	{
		uint64_t refcnt;
		struct
		{
			uint32_t atomic_refcnt;
			uint32_t nonatomic_refcnt;
		};
	};

	dual_refcnt_t(unsigned v = 1)
	{
		atomic_refcnt = v;
		nonatomic_refcnt = 0;
	}

	bool is_zero()
	{
		if(sizeof(void*) == 8)
			return *(volatile uint64_t*)&refcnt == 0ULL;
		else
		{
			uint64_t v;
			do
			{
				v = refcnt;
			}
			while(!__sync_bool_compare_and_swap(&refcnt, v, v));
			return v == 0ULL;
		}
	}

	unsigned add_ref()
	{
		//printf("%p add_ref at %u %u\n", this, atomic_refcnt, nonatomic_refcnt);
		p_atomic_inc((int32_t*)&atomic_refcnt);
		return atomic_refcnt + nonatomic_refcnt;
	}

	unsigned release()
	{
		//printf("%p release at %u %u\n", this, atomic_refcnt, nonatomic_refcnt);
		if(p_atomic_dec_zero((int32_t*)&atomic_refcnt) && !nonatomic_refcnt && is_zero())
			return 0;
		unsigned v = atomic_refcnt + nonatomic_refcnt;
		return v ? v : 1;
	}

	void nonatomic_add_ref()
	{
		//printf("%p nonatomic_add_ref at %u %u\n", this, atomic_refcnt, nonatomic_refcnt);
		++nonatomic_refcnt;
	}

	unsigned nonatomic_release()
	{
		//printf("%p nonatomic_release at %u %u\n", this, atomic_refcnt, nonatomic_refcnt);
		if(!--nonatomic_refcnt)
		{
			__sync_synchronize();
			if(!atomic_refcnt && is_zero())
				return 0;
		}
		return 1;
	}
};
#else
// this will result in atomic operations being used while they could have been avoided
#ifdef __i386__
#warning Compile for 586+ using GCC to improve the performance of the Direct3D 10/11 state tracker
#endif
typedef refcnt_t dual_refcnt_t;
#endif

#define IID_MAGIC_DELETE_THIS (*(const IID*)((intptr_t)-(int)(sizeof(IID) - 1)))

template<typename Base = IUnknown, typename RefCnt = refcnt_t>
struct GalliumComObject : public Base
{
	RefCnt refcnt;

	GalliumComObject()
	{}

	/* DO NOT CALL this from externally called non-virtual functions in derived classes, since
	 * the vtable position depends on the COM interface being implemented
	 */
	virtual ~GalliumComObject()
	{}

	inline ULONG add_ref()
	{
		return refcnt.add_ref();
	}

	inline ULONG release()
	{
		ULONG v = refcnt.release();
		if(!v)
		{
			/* this will call execute "delete this", using the correct vtable slot for the destructor */
			/* see the initial comment for an explaination of this magic trick */
			this->QueryInterface(IID_MAGIC_DELETE_THIS, 0);
			return 0;
		}
		return v;
	}

	inline void nonatomic_add_ref()
	{
		refcnt.nonatomic_add_ref();
	}

	inline void nonatomic_release()
	{
		if(!refcnt.nonatomic_release())
		{
			/* this will execute "delete this", using the correct vtable slot for the destructor */
			/* see the initial comment for an explaination of this magic trick */
			this->QueryInterface(IID_MAGIC_DELETE_THIS, 0);
		}
	}

	inline HRESULT query_interface(REFIID riid, void **ppvObject)
	{
		if(com_traits<Base>::is_self_or_ancestor(riid))
		{
			// must be the virtual AddRef, since it is overridden by some classes
			this->AddRef();
			*ppvObject = this;
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return add_ref();
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return release();
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		void **ppvObject)
	{
		/* see the initial comment for an explaination of this magic trick */
		if(&riid == &IID_MAGIC_DELETE_THIS)
		{
			delete this;
			return 0;
		}
		if(!this)
			return E_INVALIDARG;
		if(!ppvObject)
			return E_POINTER;
		return query_interface(riid, ppvObject);
	}
};

template<typename BaseClass, typename SecondaryInterface>
struct GalliumMultiComObject : public BaseClass, SecondaryInterface
{
	// we could avoid this duplication, but the increased complexity to do so isn't worth it
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return BaseClass::add_ref();
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return BaseClass::release();
	}

	inline HRESULT query_interface(REFIID riid, void **ppvObject)
	{
		HRESULT hr = BaseClass::query_interface(riid, ppvObject);
		if(SUCCEEDED(hr))
			return hr;
		if(com_traits<SecondaryInterface>::is_self_or_ancestor(riid))
		{
			// must be the virtual AddRef, since it is overridden by some classes
			this->AddRef();
			*ppvObject = (SecondaryInterface*)this;
			return S_OK;
		}
		else
			return E_NOINTERFACE;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID riid,
		void **ppvObject)
	{
		/* see the initial comment for an explaination of this magic trick */
		if(&riid == &IID_MAGIC_DELETE_THIS)
		{
			delete this;
			return 0;
		}
		if(!this)
			return E_INVALIDARG;
		if(!ppvObject)
			return E_POINTER;
		return query_interface(riid, ppvObject);
	}
};

template<typename T, typename Traits>
struct refcnt_ptr
{
	T* p;

	refcnt_ptr()
	: p(0)
	{}

	void add_ref() {Traits::add_ref(p);}
	void release() {Traits::release(p);}

	template<typename U, typename UTraits>
	refcnt_ptr(const refcnt_ptr<U, UTraits>& c)
	{
		*this = static_cast<U*>(c.ref());
	}

	~refcnt_ptr()
	{
		release();
	}

	void reset(T* q)
	{
		release();
		p = q;
	}

	template<typename U, typename UTraits>
	refcnt_ptr& operator =(const refcnt_ptr<U, UTraits>& q)
	{
		return *this = q.p;
	}

	template<typename U>
	refcnt_ptr& operator =(U* q)
	{
		release();
		p = static_cast<T*>(q);
		add_ref();
		return *this;
	}

	T* ref()
	{
		add_ref();
		return p;
	}

	T* steal()
	{
		T* ret = p;
		p = 0;
		return ret;
	}

	T* operator ->()
	{
		return p;
	}

	const T* operator ->() const
	{
		return p;
	}

	T** operator &()
	{
		assert(!p);
		return &p;
	}

	bool operator !() const
	{
		return !p;
	}

	typedef T* refcnt_ptr::*unspecified_bool_type;

	operator unspecified_bool_type() const
	{
		return p ? &refcnt_ptr::p : 0;
	}
};

struct simple_ptr_traits
{
	static void add_ref(void* p) {}
	static void release(void* p) {}
};

struct com_ptr_traits
{
	static void add_ref(void* p)
	{
		if(p)
			((IUnknown*)p)->AddRef();
	}

	static void release(void* p)
	{
		if(p)
			((IUnknown*)p)->Release();
	}
};

template<typename T>
struct ComPtr : public refcnt_ptr<T, com_ptr_traits>
{
	template<typename U, typename UTraits>
	ComPtr& operator =(const refcnt_ptr<U, UTraits>& q)
	{
		return *this = q.p;
	}

	template<typename U>
	ComPtr& operator =(U* q)
	{
		this->release();
		this->p = static_cast<T*>(q);
		this->add_ref();
		return *this;
	}
};

template<typename T, typename TTraits, typename U, typename UTraits>
bool operator ==(const refcnt_ptr<T, TTraits>& a, const refcnt_ptr<U, UTraits>& b)
{
	return a.p == b.p;
}

template<typename T, typename TTraits, typename U>
bool operator ==(const refcnt_ptr<T, TTraits>& a, U* b)
{
	return a.p == b;
}

template<typename T, typename TTraits, typename U>
bool operator ==(U* b, const refcnt_ptr<T, TTraits>& a)
{
	return a.p == b;
}

template<typename T, typename TTraits, typename U, typename UTraits>
bool operator !=(const refcnt_ptr<T, TTraits>& a, const refcnt_ptr<U, UTraits>& b)
{
	return a.p != b.p;
}

template<typename T, typename TTraits, typename U>
bool operator !=(const refcnt_ptr<T, TTraits>& a, U* b)
{
	return a.p != b;
}

template<typename T, typename TTraits, typename U>
bool operator !=(U* b, const refcnt_ptr<T, TTraits>& a)
{
	return a.p != b;
}

template<bool threadsafe>
struct maybe_mutex_t;

template<>
struct maybe_mutex_t<true>
{
	pipe_mutex mutex;

	maybe_mutex_t()
	{
		pipe_mutex_init(mutex);
	}

	void lock()
	{
		pipe_mutex_lock(mutex);
	}

	void unlock()
	{
		pipe_mutex_unlock(mutex);
	}
};

template<>
struct maybe_mutex_t<false>
{
	void lock()
	{
	}

	void unlock()
	{
	}
};

typedef maybe_mutex_t<true> mutex_t;

template<typename T>
struct lock_t
{
	T& mutex;
	lock_t(T& mutex)
	: mutex(mutex)
	{
		mutex.lock();
	}

	~lock_t()
	{
		mutex.unlock();
	}
};

struct c_string
{
	const char* p;
	c_string(const char* p)
	: p(p)
	{}

	operator const char*() const
	{
		return p;
	}
};

static inline bool operator ==(const c_string& a, const c_string& b)
{
	return !strcmp(a.p, b.p);
}

static inline bool operator !=(const c_string& a, const c_string& b)
{
	return strcmp(a.p, b.p);
}

static inline size_t raw_hash(const char* p, size_t size)
{
	size_t res;
	if(sizeof(size_t) >= 8)
		res = (size_t)14695981039346656037ULL;
	else
		res = (size_t)2166136261UL;
	const char* end = p + size;
	for(; p != end; ++p)
	{
		res ^= (size_t)*p;
		if(sizeof(size_t) >= 8)
			res *= (size_t)1099511628211ULL;
		else
			res *= (size_t)16777619UL;
	}
	return res;
};

template<typename T>
static inline size_t raw_hash(const T& t)
{
	return raw_hash((const char*)&t, sizeof(t));
}

// TODO: only tested with the gcc libstdc++, might not work elsewhere
namespace std
{
#ifndef _MSC_VER
	namespace tr1
	{
#endif
		template<>
		struct hash<GUID> : public std::unary_function<GUID, size_t>
		{
			inline size_t operator()(GUID __val) const;
		};

		inline size_t hash<GUID>::operator()(GUID __val) const
		{
			return raw_hash(__val);
		}

		template<>
		struct hash<c_string> : public std::unary_function<c_string, size_t>
		{
			inline size_t operator()(c_string __val) const;
		};

		inline size_t hash<c_string>::operator()(c_string __val) const
		{
			return raw_hash(__val.p, strlen(__val.p));
		}

		template<typename T, typename U>
		struct hash<std::pair<T, U> > : public std::unary_function<std::pair<T, U>, size_t>
		{
			inline size_t operator()(std::pair<T, U> __val) const;
		};

		template<typename T, typename U>
		inline size_t hash<std::pair<T, U> >::operator()(std::pair<T, U> __val) const
		{
			std::pair<size_t, size_t> p;
			p.first = hash<T>()(__val.first);
			p.second = hash<U>()(__val.second);
			return raw_hash(p);
		}
#ifndef _MSC_VER
	}
#endif
}

template<typename Base, typename RefCnt = refcnt_t>
struct GalliumPrivateDataComObject : public GalliumComObject<Base, RefCnt>
{
	typedef std::unordered_map<GUID, std::pair<void*, unsigned> > private_data_map_t;
	private_data_map_t private_data_map;
	mutex_t private_data_mutex;

	~GalliumPrivateDataComObject()
	{
		for(private_data_map_t::iterator i = private_data_map.begin(), e = private_data_map.end(); i != e; ++i)
		{
			if(i->second.second == ~0u)
				((IUnknown*)i->second.first)->Release();
			else
				free(i->second.first);
		}
	}

	HRESULT get_private_data(
		REFGUID guid,
		UINT *pDataSize,
		void *pData)
	{
		lock_t<mutex_t> lock(private_data_mutex);
		private_data_map_t::iterator i = private_data_map.find(guid);
		*pDataSize = 0;
		if(i == private_data_map.end())
			return DXGI_ERROR_NOT_FOUND;
		if(i->second.second == ~0u)
		{
			/* TODO: is GetPrivateData on interface data supposed to do this? */
			if(*pDataSize < sizeof(void*))
				return E_INVALIDARG;
			if(pData)
			{
				memcpy(pData, &i->second.first, sizeof(void*));
				((IUnknown*)i->second.first)->AddRef();
			}
			*pDataSize = sizeof(void*);
		}
		else
		{
			unsigned size = std::min(*pDataSize, i->second.second);
			if(pData)
				memcpy(pData, i->second.first, size);
			*pDataSize = size;
		}
		return S_OK;
	}

	HRESULT set_private_data(
		REFGUID guid,
		UINT DataSize,
		const void *pData)
	{
		void* p = 0;

		if(DataSize && pData)
		{
			p = malloc(DataSize);
			if(!p)
				return E_OUTOFMEMORY;
		}

		lock_t<mutex_t> lock(private_data_mutex);
		std::pair<void*, unsigned>& v = private_data_map[guid];
		if(v.first)
		{
			if(v.second == ~0u)
				((IUnknown*)v.first)->Release();
			else
				free(v.first);
		}
		if(DataSize && pData)
		{
			memcpy(p, pData, DataSize);
			v.first = p;
			v.second = DataSize;
		}
		else
			private_data_map.erase(guid);
		return S_OK;
	}

	HRESULT set_private_data_interface(
		REFGUID guid,
		const IUnknown *pData)
	{
		lock_t<mutex_t> lock(private_data_mutex);
		std::pair<void*, unsigned>& v = private_data_map[guid];
		if(v.first)
		{
			if(v.second == ~0u)
				((IUnknown*)v.first)->Release();
			else
				free(v.first);
		}
		if(pData)
		{
			((IUnknown*)pData)->AddRef();
			v.first = (void*)pData;
			v.second = ~0;
		}
		else
			private_data_map.erase(guid);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
		REFGUID guid,
		UINT *pDataSize,
		void *pData)
	{
		return get_private_data(guid, pDataSize, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
		REFGUID guid,
		UINT DataSize,
		const void *pData)
	{
		return set_private_data(guid, DataSize, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
		REFGUID guid,
		const IUnknown *pData)
	{
		return set_private_data_interface(guid, pData);
	}
};

template<typename BaseClass, typename SecondaryInterface>
struct GalliumMultiPrivateDataComObject : public GalliumMultiComObject<BaseClass, SecondaryInterface>
{
	// we could avoid this duplication, but the increased complexity to do so isn't worth it
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(
		REFGUID guid,
		UINT *pDataSize,
		void *pData)
	{
		return BaseClass::get_private_data(guid, pDataSize, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(
		REFGUID guid,
		UINT DataSize,
		const void *pData)
	{
		return BaseClass::set_private_data(guid, DataSize, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(
		REFGUID guid,
		const IUnknown *pData)
	{
		return BaseClass::set_private_data_interface(guid, pData);
	}
};

#define DXGI_FORMAT_COUNT 116
extern pipe_format dxgi_to_pipe_format[DXGI_FORMAT_COUNT];
extern DXGI_FORMAT pipe_to_dxgi_format[PIPE_FORMAT_COUNT];

void init_pipe_to_dxgi_format();

COM_INTERFACE(IGalliumDevice, IUnknown);
COM_INTERFACE(IGalliumAdapter, IUnknown);
COM_INTERFACE(IGalliumResource, IUnknown);

// used to make QueryInterface know the IIDs of the interface and its ancestors
COM_INTERFACE(IDXGIObject, IUnknown)
COM_INTERFACE(IDXGIDeviceSubObject, IDXGIObject)
COM_INTERFACE(IDXGISurface, IDXGIDeviceSubObject)
COM_INTERFACE(IDXGIOutput, IDXGIObject)
COM_INTERFACE(IDXGIAdapter, IDXGIObject)
COM_INTERFACE(IDXGISwapChain, IDXGIDeviceSubObject)
COM_INTERFACE(IDXGIFactory, IDXGIObject)
COM_INTERFACE(IDXGIDevice, IDXGIObject)
COM_INTERFACE(IDXGIResource, IDXGIDeviceSubObject)
COM_INTERFACE(IDXGISurface1, IDXGISurface)
COM_INTERFACE(IDXGIDevice1, IDXGIDevice)
COM_INTERFACE(IDXGIAdapter1, IDXGIAdapter)
COM_INTERFACE(IDXGIFactory1, IDXGIFactory)

template<typename Base>
struct GalliumDXGIDevice : public GalliumMultiPrivateDataComObject<Base, IDXGIDevice1>
{
	ComPtr<IDXGIAdapter> adapter;
	int priority;
	unsigned max_latency;

	GalliumDXGIDevice(IDXGIAdapter* p_adapter)
	{
		adapter = p_adapter;
	}

	virtual HRESULT STDMETHODCALLTYPE GetParent(
		REFIID riid,
		void **ppParent)
	{
		return adapter.p->QueryInterface(riid, ppParent);
	}

	virtual HRESULT STDMETHODCALLTYPE GetAdapter(
		IDXGIAdapter **pAdapter)
	{
		*pAdapter = adapter.ref();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE QueryResourceResidency(
		IUnknown *const *ppResources,
		DXGI_RESIDENCY *pResidencyStatus,
		UINT NumResources)
	{
		for(unsigned i = 0; i < NumResources; ++i)
			pResidencyStatus[i] = DXGI_RESIDENCY_FULLY_RESIDENT;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(
		INT Priority)
	{
		priority = Priority;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(
		INT *pPriority)
	{
		*pPriority = priority;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(
		UINT *pMaxLatency
	)
	{
		*pMaxLatency = max_latency;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(
		UINT MaxLatency)
	{
		max_latency = MaxLatency;
		return S_OK;
	}
};

COM_INTERFACE(ID3D10Blob, IUnknown);

/* NOTE: ID3DBlob implementations may come from a Microsoft native DLL
 * (e.g. d3dcompiler), or perhaps even from the application itself.
 *
 * Hence, never try to access the data/size members directly, which is why they are private.
 * In internal code, use std::pair<void*, size_t> instead of this class.
 */
class GalliumD3DBlob : public GalliumComObject<ID3DBlob>
{
	void* data;
	size_t size;

public:
	GalliumD3DBlob(void* data, size_t size)
	: data(data), size(size)
	{}

	~GalliumD3DBlob()
	{
		free(data);
	}

	virtual LPVOID STDMETHODCALLTYPE GetBufferPointer()
	{
		return data;
	}

	virtual SIZE_T STDMETHODCALLTYPE GetBufferSize()
	{
		return size;
	}
};

#endif /* D3D1XSTUTIL_H_ */
