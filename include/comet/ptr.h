/** \file
  * Provides the com_ptr type.
  */
/*
 * Copyright � 2000-2002 Sofus Mortensen, Michael Geddes
 *
 * This material is provided "as is", with absolutely no warranty
 * expressed or implied. Any use is at your own risk. Permission to
 * use or copy this software for any purpose is hereby granted without
 * fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is
 * granted, provided the above notices are retained, and a notice that
 * the code was modified is included with the above copyright notice.
 *
 * This header is part of Comet version 2.
 * https://github.com/alamaison/comet
 */
/*! \addtogroup COMType
 */
//@{

/** \page cometcomptr Comet com_ptr
 * \section cometcomptroverview Overview
 * The \link comet::com_ptr com_ptr \endlink type is essentially a reference
 * counting wrapper for objects that support AddRef / Release in a way that
 * is STL container compatible.
 *
 * It also provides a mechanism for doing casting (QueryInterface) assignments
 * as well as for doing assignment-compatible assignments and for providing
 * smart wrappers for the interfaces.
 *
 * \section cometcomptrwrapping What Can Be Wrapped?
 *
 * The com_ptr has been designed to allow most classes to be wrapped, specifically
 * it allows for wrapping ::IUnknown based interfaces, and also <b>coclass</b> implementations.
 *
 * It should be noted that some coclasses can have two implementations of
 * ::IUnknown (aggregateable coclasses being a prime example).  These classes
 * support \link comet::implement_qi::get_unknown get_unknown() \endlink which allows
 * the com_ptr to get a the unknown responsible for the lifetime management of
 * the object.
 *
 * \section cometcomptrassign Assigning Pointers
 *
 * There are three ways of assigning and constructing com_ptr objects, depending on your
 * objective; direct assignment and the two cast operators com_cast and try_cast.
 *
 * \subsection cometcomptrassigndirect Direct Assignment

 * The first way is trivial, but quite important and is the simple
 * assignment of one com_ptr to another.  This is done in such a way as to make
 * it possible to assign assignment compatible interfaces (eg an ::IUnknown pointer can be
 * assigned from an ::IDispatch pointer).  A compile-time error will occur if
 * the pointers are not compatible.
 *
 * Unfortunately there are certain circumstances that MSVC6 compiler does not
 * provide a complete enough template instantiation backtrace in order to be
 * able to directly work out an offending assignment.  Our appologies, but a
 * decent compiler would be more accomodating!
 *
 * \subsection cometcomptrassignquery Casting Assignment
 *
 * To cause a COM cast or ::QueryInterface to happen, you need to use the
 * wrapper functions com_cast and try_cast.  The cause a QueryInterface on
 * both assignment and construction, allowing you the choice between having
 * an exception thrown on failure (try_cast) or to silently fail leaving the
 * object NULL (com_cast).
 *
 * \code

			com_ptr<IViewObject> viewobj = com_cast( obj ); // <-- non-throwing query-interface
			if (viewobj.is_null())
			{
				com_ptr<IViewObject2> viewobj2 = try_cast( obj ); // <-- throwing query-interface
				viewobj = viewobj2;	// <-- Assignment between assignment compatible types.
			}
 * \endcode

 * This shows the three different ways of assigning.  The first uses <b>com_cast</b>
 * that causes the assignment to do a QueryInterface, but does not throw an
 * error.  The second uses <b>try_cast</b> that again causes the assignment to
 * do a QueryInterface, but causes errors to be thrown.  The third assignment is
 * for assignment compatible objects only, and will cause a compiler-error
 * unless (in this case) IViewObject2 inherits off IViewObject (which it does).</p>
 *
 * \subsection cometcomptrassignvariant  Assignment From a variant_t
 *
 * Assignment to a com_ptr from a variant_t must be done by either com_cast or
 * try_cast, as any assignment from a variant is effectively a cast.
 *
 * \section cometcomptrsmartwrapper Smart Wrappers
 *
 * The com_ptr \link comet::com_ptr::operator-> operator-> \endlink provides wrapped
 * access to the methods on the interface.  For interfaces that have been
 * generated by tlb2h, the interface returned is a wrapper that allows access
 * only to the wrapped methods, otherwise, access to the raw interface is
 * default.
 *
 * The design of the wrappers allows for system interfaces to be wrapped as
 * well.  There is an \link comet::com_ptr< ::IDispatch> IDispatch wrapper \endlink
 * defined in comet/dispatch.h that provides methods to call functions and
 * property accessors by name or by dispatch id.
 *
 * There is also a wrapper for the API TypeLibrary information interfaces
 * defined in comet/tlbinfo.h.  In addition to providing some simple wrappers
 * to hide the raw COM types, it also provides accessor wrappers to the structs
 * that are allocated/deallocated by the interfaces.
 *
 * \subsection cometcomptrsmartwrapperdetail Implementation
 * It is <b>not</b> necessary to know how the wrappers are implemented in order
 * to use them, however the technique is quite interesting.
 *
 * They key to the wrappers is template specialisation.  The operator->
 * returns the 'this' pointer with a <kbd>reinterpret_cast</kbd> to a wrap_t
 * struct instantiated to the interface contained by the com_ptr.  The default
 * wrap_t template definition is to inherit from the interface, thus giving
 * direct access to methods.
 *
 * The tlb2h generated headers (as well as comet/tlbinfo.h and
 * a0omet/dispatch.h) provide alternate specialisations for wrap_t that wrap the
 * arguments and call the real methods (which would be Invoke if it is an dispinterface)
 * by using reinterpret_cast to cast back to the original interface.
 *
 * \section cometcomptraccess Raw COM Access
 * Whether calling raw COM interfaces, or trying to understand how the wrappers
 * work, you will come across the \ref cometrawcomaccess methods.  These are standard
 * across the types (see \ref cometrawcomaccess), however com_ptr classes also
 * have a raw() method that is equivalent to get() and proivdes access to the
 * raw COM methods.
 */
 //@}

#ifndef COMET_PTR_H
#define COMET_PTR_H

#include <comet/config.h>

#include <algorithm>

#include <comet/error_fwd.h>
#include <comet/typelist.h>
#include <comet/interface.h>
#include <comet/common.h>
#include <comet/uuid_fwd.h>

// Primary namespace for comet
namespace comet {
	class uuid_t;

	template<typename Itf> struct wrap_t : public Itf {};

	template<> struct wrap_t< ::IUnknown> {};

	template<typename Itf> Itf* raw( wrap_t<Itf>* p ) { return reinterpret_cast<Itf*>(p); }

//	template<typename Itf> struct prop_wrapper : public Itf {};

	// Forward declaration
	class variant_t;
	template<typename Itf> class com_ptr;
	class identity_ptr;
	namespace thread_model{
		enum thread_model_t;
	}
	template<typename T, enum thread_model::thread_model_t TM, COMET_LIST_TEMPLATE_0 > struct coclass;
	template<class C> struct aggregate_inner_unknown ;

	// comet implementation details
	namespace impl {

		template<typename Itf> class com_cast_t {
			public:
				explicit com_cast_t(Itf* p) : ptr_(p) {};
				Itf* get() const { return ptr_; }
			private:
				Itf* ptr_;
				com_cast_t& operator=(const com_cast_t&);
		};

		template<> class com_cast_t<variant_t> {
		public:
			explicit com_cast_t(const variant_t& v) : val_(v) {};
			const variant_t& get() const { return val_; }
		private:
			const variant_t& val_;
			com_cast_t& operator=(const com_cast_t&);
		};

		/** Allow calling specific, known ambiguous IUnknown implementations.
		* \param B \b true Call via get_unknown. \b false Call directly
		*/
		template <bool B>
		struct IUnknown_caller
		{
			template< typename T > static inline long AddRef( T *y)
			{ return y->AddRef(); }
			template <typename T> static inline long Release( T *y)
			{ return y->Release(); }
			template <typename T> static inline HRESULT QueryInterface( T *y, REFIID iid, void  **NewIface)
			{ return y->QueryInterface(iid, (void **)NewIface);	}
		};

		template <>
		struct IUnknown_caller<true>
		{
			template<typename T> static inline long AddRef( T *y)
			{ return y->get_unknown()->AddRef(); }
			template< typename T> static inline long Release( T *y)
			{ return y->get_unknown()->Release(); }
			template <typename T> static inline HRESULT QueryInterface( T *y, REFIID iid, void  **NewIface)
			{ return y->get_unknown()->QueryInterface(iid, NewIface); }
		};
//		template<typename ITF_LIST> struct implement_internal_qi;

		/** Namespace for dummy functions for allowing IUnknown choice to be
		* deferred to the get_unknown() function.
		*/
		namespace unknown_choice
		{
//			template<typename T, enum thread_model::thread_model_t TM> long dummy_( coclass<T,TM> *);
//			template<typename ITF_LIST> long dummy_(implement_internal_qi<ITF_LIST> *);
			template<class C> long dummy_(aggregate_inner_unknown<C> *);
			char dummy_( ... );
		}

		/** \internal
		*/
		template <typename C>
		struct iunknown_chooser
		{

			enum { matches = (sizeof( unknown_choice::dummy_( static_cast< C *>(0) ))==sizeof(long)) };
		};

	}
	/*! \addtogroup COMType
	 */
	//@{

	//! Cast com_ptr.
	/*! Allows QueryInterface when casting to different com_ptr type.
		\code
			com_ptr<IFoo> foo;
			com_ptr<IBar> bar;
			bar = com_cast(foo);
			if (!bar.is_null()) {
				// Cast is ok.
				bar->DoTheThing();
			}
		\endcode
	 * \param t com_ptr to cast
	 * \relates com_ptr
	*/
	template<typename Itf> inline impl::com_cast_t<Itf> com_cast(const com_ptr<Itf>& t) { return impl::com_cast_t<Itf>(t.get()); }
	template<typename Itf> inline impl::com_cast_t<Itf> com_cast(Itf* t) { return impl::com_cast_t<Itf>(t); }
	inline impl::com_cast_t<variant_t> com_cast(const variant_t& v) { return impl::com_cast_t<variant_t>(v); }

	namespace impl {
		template<typename Itf> class try_cast_t {
			public:
				explicit try_cast_t(Itf* p) : ptr_(p) {};
				Itf* get() const { return ptr_; }
			private:
				Itf* ptr_;
				try_cast_t& operator=(const try_cast_t&);
		};

		template<> class try_cast_t<variant_t> {
		public:
			explicit try_cast_t(const variant_t& v) : val_(v) {};
			const variant_t& get() const { return val_; }
		private:
			const variant_t& val_;
			try_cast_t& operator=(const try_cast_t&);
		};
	}


	namespace impl {

		//!  IUnknown wrapper.
		/** Hides the members of IUnknown.
		 * \relates com_ptr
		*/
		template<typename Itf>	class safe_interface : public Itf {
		private:
			HRESULT __stdcall QueryInterface(REFIID, void**);
			ULONG __stdcall AddRef();
			ULONG __stdcall Release();
		};

	}

	//! Represents the identity Unknown of an object.
	/** Is the only really efficient and safe way of representing an object for
	 *  comparisons.
	 */
	class identity_ptr
	{
		public:
			//! Default constructor.
			/** Initialises the pointer to Null.
			 */
			identity_ptr() throw() : ptr_(NULL){}
			//! Destructor.
			/** Releases the pointer.
			 */
			~identity_ptr() throw() { release();}

			//! Copy constructor.
			/*! Simple pointer copy.  They are both already identity unknowns.
			 */
			identity_ptr( const identity_ptr &rhs) : ptr_(rhs.ptr_) {addref();}

			//! Constructor from Try Cast.
			/*! Used for construction from any Interface pointer.  Always QIs to
			 * guarantee this is the identity.
			 */
			template<typename Itf>
			explicit identity_ptr( const impl::try_cast_t<Itf> &x ) throw(com_error)
				: ptr_(NULL)
			{
				IUnknown *p=static_cast<IUnknown*>(x.get());
				if (p != NULL)
					p->QueryInterface(uuidof< ::IUnknown >(), reinterpret_cast<void**>(&ptr_)) | raise_exception;
			}

			//! Constructor from variant_t Try Cast.
			/*! Used for construction from a variant.  Always QIs to
			 * guarantee this is the identity.
			 */
			inline explicit identity_ptr(const impl::try_cast_t<variant_t>& v) throw(com_error);

			//! Constructor from Com Cast.
			/*! Used for construction from any Interface pointer.  Always QIs to
			 * guarantee this is the identity.
			 */
			template<typename Itf>
			explicit identity_ptr( const impl::com_cast_t<Itf> &x ) throw(com_error)
				: ptr_(NULL)
			{
				IUnknown *p=static_cast<IUnknown*>(x.get());
				if (p != NULL)
					p->QueryInterface(uuidof< ::IUnknown >(), reinterpret_cast<void**>(&ptr_));
			}
			//! Constructor from variant_t com_cast.
			/*! Used for construction from a variant.  Always QIs to
			 * guarantee this is the identity.
			 */
			inline explicit identity_ptr(const impl::com_cast_t<variant_t>& v) throw(com_error);

			//! Constructions of null pointer
			/*!
				\param null
					Only 0 is valid. Any other value will cause E_POINTER to be thrown.

				\exception com_error
					Throws E_POINTER if a non-zero value is specified.
			*/
			explicit identity_ptr(int null) throw(com_error)
				: ptr_(NULL)
			{ if(null != 0) raise_exception(E_POINTER); }

			//! Copy assignment.
			/*! Don't QI as this is definitely identity already.
			 */
			identity_ptr &operator =( const identity_ptr &rhs) throw()
			{
				identity_ptr tmp(rhs);
				swap(tmp);
				return *this;
			}

			//! Other assignment.
			/*! Always QIs. Handles other assignments that have
			 * available constructors.
			 */
			template<typename T>
			identity_ptr &operator =(const T &rhs) throw(com_error)
			{
				identity_ptr tmp(rhs);
				swap(tmp);
				return *this;
			}
			//! Null assignment
			/*!
				Only null is allowed as argument. Attempting to assign a non-zero value will result in E_POINTER (wrapped in com_error) being thrown.

				\exception com_error
					Throw E_POINTER if a non-zero value is specified.
			*/
			identity_ptr& operator=(int null) throw(com_error)
			{ if(null != 0) raise_exception(E_POINTER); release(); return *this; }

			/// Get at the raw pointer.
			IUnknown *get() const throw() { return  ptr_; }
			/// Pass to an [in] parameter.
			IUnknown *in() const throw() { return ptr_; }
			/// Get at raw interface.
			IUnknown *raw() const throw() { return ptr_; }

			//! Returns true if the wrapped pointer is null.
			bool is_null() const throw()
			{ return (ptr_ ==NULL); }

			//! Null comparison
			/*!
				Only comparison with a value of zero is allowed. Non-zero values will result
				in E_POINTER (wrapped in com_error) being thrown.

				\exception com_error
			*/
			bool operator==(int null) const
			{ if (null != 0) raise_exception(E_POINTER); return is_null(); }
			//! Null comparison
			/*!
				Only comparison with a value of zero is allowed. Non-zero values will result
				in E_POINTER (wrapped in com_error) being thrown.

				\exception com_error
			*/
			bool operator!=(int null) const
			{ if (null != 0) raise_exception(E_POINTER); return !is_null();	}

			/**! \name Pointer Comparison
			  */
			//@{
			bool operator<(const identity_ptr& x) const throw()
			{ return ptr_ < x.ptr_; }

			bool operator>(const identity_ptr& x) const throw()
			{ return ptr_ > x.ptr_; }


			bool operator<=(const identity_ptr& x) const throw()
			{ return ptr_ <= x.ptr_; }

			bool operator>=(const identity_ptr& x) const throw()
			{ return ptr_ >= x.ptr_; }

			bool operator==(const identity_ptr& x) const throw()
			{ return (ptr_ == x.ptr_); }

			bool operator!=(const identity_ptr& x) const throw()
			{ return ptr_ != x.ptr_; }
			//@}

		protected:
			IUnknown *ptr_;

			void swap(identity_ptr& x) throw()
			{ std::swap(ptr_, x.ptr_); }

			void addref() const throw()
			{ if (ptr_ != NULL) ptr_->AddRef(); }

			void release() throw()
			{ if (ptr_ != NULL) { ptr_->Release(); ptr_ = NULL; } }

		private:
			// Can't use an identity_ptr for out, inout, use com_ptr<IUnknown>
			IUnknown **inout();
			IUnknown *out();

	};

	//! Interface smart pointer
	/*! \note All members are exception safe.
	 * \sa cometcomptr
	*/
	template<typename Itf> class com_ptr
	{
	public:
		const uuid_t& iid() const throw()
		{ return uuidof<Itf>();	}

	private:
		enum{ chooser_matches_= impl::iunknown_chooser<Itf>::matches };
		typedef typename impl::IUnknown_caller< chooser_matches_ > Unknown_caller_;

		void addref() const throw()
		{ if (ptr_) Unknown_caller_::AddRef(ptr_); }

		void release() throw()
		{ if (ptr_) { Unknown_caller_::Release(ptr_); ptr_ = 0; } }

	public:
		//! Type of pointer to wrapped interface
		typedef Itf* interface_pointer;

		//! Safe interface pointer.
		/*!
			Interface pointer where the methods from IUnknown have been hidden.
		*/
#ifndef COMET_USE_RAW_WRAPPERS
		typedef wrap_t<Itf>* safe_interface_pointer;
#else
		typedef impl::safe_interface< Itf >* safe_interface_pointer;
#endif

	private:
		interface_pointer ptr_;

	public:
		//! Default constructor
		/*!
			Initialises the wrapped pointer to null.
		*/
		com_ptr() throw() : ptr_(NULL) {}

		//! Destructor
		/*!
			Calls Release() on pointer if necessary.
		*/
		~com_ptr() throw()
		{ release(); }

		//! Upcasting constructor
		/*!
			Used for upcasting interface pointer without invocation of QueryInterface.

			This contructor only allows com_ptr upcasts.

			\param x
				com_ptr to cast.
		*/

		// If you are getting:
		//        error C2440: '=' : cannot convert from 'struct ...
		//
		// Use either try_cast or com_cast to cast from one interface to another
		// example:
		//
		//    com_ptr<IFoo> p = try_cast( q );
		//
		// or
		//
		//    com_ptr<IFoo> p = com_cast( q );
		template<typename Itf2> com_ptr(const com_ptr<Itf2>& x) throw()
		{ ptr_ = (Itf2*)x.get(); addref(); }

		//! QueryInterface construction
		/*!
			Uses QueryInterface to query for interface of type Itf. If the pointer is incompatible, the pointer will be initialised to null.

			\code
			com_ptr<IFoo> foo;
			com_ptr<IBar> bar( com_cast(foo) );
			\endcode

			\param x
				com_ptr to cast
		*/
		template<typename Itf2> com_ptr(const impl::com_cast_t<Itf2>& x) throw()
		{ ptr_ = NULL; Itf2* p = x.get(); if (p != NULL) Unknown_caller_::QueryInterface(p, iid(), reinterpret_cast<void**>(&ptr_)); }
		//                                                                   ^ Note: no error checking!!!

		//! QueryInterface construction
		/*!
			Uses QueryInterface to query for interface of type Itf. Should the pointer be incompatible, com_error will be thrown.

			\code
			com_ptr<IFoo> foo = Foo::create();
			com_ptr<IBar> bar( try_cast(foo) );
			\endcode

			\param x
				com_ptr to cast

			\exception com_error
				Throws E_NOINTERFACE if cast fails.
				Throws E_POINTER if pointer is zero.
		*/
		template<typename Itf2> com_ptr(const impl::try_cast_t<Itf2>& x) throw(com_error)
			: ptr_(NULL)
		{
			Itf2* p = x.get(); if (p == NULL) return;
			Unknown_caller_::QueryInterface(p, iid(), reinterpret_cast<void**>(&ptr_)) | raise_exception;
			if (ptr_ == NULL) raise_exception(E_NOINTERFACE);
		}

		//! Copy constructor
		/*!
			\param x com_ptr to copy.
		*/
		com_ptr(const com_ptr& x) throw()
			: ptr_(x.ptr_)
		{ addref(); }

		//! Copy from identity_unknown class.
		/** Itf  must be IUnknown for this to work. Otherwise use try_cast or
		 * com_cast.
		 */
		com_ptr( const identity_ptr &x) throw()
			: ptr_(x.ptr_)
		{ addref(); }

	private:
		// If you are getting:
		//        error C2248: '=' : cannot access private member ...
		//
		// Use either try_cast or com_cast to convert a variant_t to a com_ptr.
		// example:
		//
		//    com_ptr<IFoo> p = try_cast( v );
		//
		// or
		//
		//    com_ptr<IFoo> p = com_cast( v );
		com_ptr(const variant_t&);
		com_ptr& operator=(const variant_t&);

	public:

		//! Construction from variant_t
		/*!
			This constructor does not throw. The wrapped pointer is initialised to null if the variant is incompatible.

			\param v
				Wrapped com_variant to construct com_ptr from
		*/
		com_ptr(const impl::com_cast_t<variant_t>& v) throw()
		{ create_nothrow(v.get()); }

		//! Construction from variant_t
		/*!
			Throws com_error should the variant be incompatible.

			\param v
				Wrapped com_variant to construct com_ptr from

			\exception com_error
				Throws E_NOINTERFACE if cast fails.
		*/
		com_ptr(const impl::try_cast_t<variant_t>& v) throw(com_error)
		{ create(v.get()); }

		//! Construction from a raw interface pointer
		/*!
			Calls AddRef, and thus does \em not take ownership of the pointer.

			\param p
				Interface pointer to construct com_ptr from.
		*/
		com_ptr(interface_pointer p) throw()
			: ptr_(p)
		{ addref();	}

		//! Attach construction of raw interface pointer
		/*!
			An attachment construction does not AddRef on the interface, and is said
			to take ownership of the interface pointer.

			\param p
				auto_attach wrapped pointer to construct com_ptr from.
		*/
		com_ptr(const impl::auto_attach_t<interface_pointer>& p) throw()
			: ptr_( p.get() )
		{}

		//! Construction from CLSID
		/*!
			\param clsid
				Class ID of coclass.
			\param dwClsContext
				Class context to create object in.

			\exception com_error
		*/
		explicit com_ptr(const uuid_t& clsid, DWORD dwClsContext = CLSCTX_ALL) throw(com_error)
		{ create(clsid, NULL, dwClsContext); }

		//! Construction of aggregated object from CLSID
		/*!
			\param clsid
				Class ID of coclass.
			\param outer
				Pointer to outer object.
			\param dwClsContext
				Class context to create object in.

			\exception com_error
		*/
		/*template<typename Itf2>*/ com_ptr(const uuid_t& clsid, const com_ptr< ::IUnknown>& outer, DWORD dwClsContext = CLSCTX_ALL) throw(com_error)
		{ create(clsid, outer, dwClsContext); }

		//! Constructions of null pointer
		/*!
			\param null
				Only 0 is valid. Any other value will cause E_POINTER to be thrown.

			\exception com_error
				Throws E_POINTER if a non-zero value is specified.
		*/
		explicit com_ptr(int null) throw(com_error)
			: ptr_(NULL)
		{ if(null != 0) raise_exception(E_POINTER); }

		//! Construction from ProgID
		/*!
			\param progid
				ProgID of class.
			\param dwClsContext
				Class context to create object in.

			\exception com_error
		*/
		explicit com_ptr(const wchar_t* progid, DWORD dwClsContext = CLSCTX_ALL) throw(com_error)
		{ create(progid, 0, dwClsContext); }

		//! Construction from ProgID
		/*!
			\param progid
				ProgID of class.
			\param outer
				Pointer to outer object.
			\param dwClsContext
				Class context to create object in.

			\exception com_error
		*/
		/*template<typename Itf2> */com_ptr(const wchar_t* progid, const com_ptr< ::IUnknown>& outer, DWORD dwClsContext = CLSCTX_ALL) throw(com_error)
		{ create(progid, outer, dwClsContext); }

		//! Construction using CoGetObject
		explicit com_ptr(const wchar_t* name, BIND_OPTS& bind_opts)
		{ create(name, bind_opts); }

		//! Arrow operator
		/*!
			Used to access methods and properties of the wrapped interface.

			\note The methods of IUnknown are deliberately hidden.
		*/
		safe_interface_pointer operator->() const throw(com_error)
		{ if( ptr_ == NULL) raise_exception(E_POINTER);	return get_safe_ptr(); }

		//! Assignment of raw interface pointer
		com_ptr& operator=(interface_pointer x) throw()
		{ com_ptr t(x);	swap(t); return *this; }

		//! QueryInterface assignment
		/*!
			In order to cast unrelated interfaces you must use either com_cast or try_cast.

			A com_cast that fails results in a null pointer assignment.

			\code
				com_ptr<IFoo> foo;
				com_ptr<IBar> bar;
				bar = com_cast(foo);
				if (!bar.is_null()) {
					// Cast is ok.
					bar->DoTheThing();
				}
			\endcode
		*/
		template<typename Itfx>	com_ptr& operator=(const impl::com_cast_t<Itfx>& x) throw()
		{ com_ptr t(x); swap(t); return *this; }

		//! QueryInterface assignment
		/*!
			In order to cast unrelated interfaces you must use either com_cast or try_cast.

			A try_cast that fails will throw E_NOINTERFACE wrapped in com_error.

			\code
				com_ptr<IFoo> foo;
				com_ptr<IBar> bar;
				try {
					bar = try_cast(foo);
					bar->DoTheThing();
				} catch (com_error&) {
					// Cast didn't work.
				}
			\endcode

			\exception com_error
				Throws E_NOINTERFACE if cast fails.
		*/
		template<typename Itf2> com_ptr& operator=(const impl::try_cast_t<Itf2>& x) throw(com_error)
		{ com_ptr t(x); swap(t); return *this; }

		template<typename Itf2>
		com_ptr& operator=(const com_ptr<Itf2>& x) throw()
		{ com_ptr t(x);	swap(t); return *this; }

		//! Default assigment operator.
		com_ptr& operator=(const com_ptr& x) throw()
		{ com_ptr t(x);	swap(t); return *this; }

		//! Null assignment
		/*!
			Only null is allowed as argument. Attempting to assign a non-zero value will result in E_POINTER (wrapped in com_error) being thrown.

			\exception com_error
				Throw E_POINTER if a non-zero value is specified.
		*/
		com_ptr& operator=(int null) throw(com_error)
		{ if(null != 0) raise_exception(E_POINTER); release(); return *this; }

		//! Attaching assignment
		/*!
			Attaches a raw interface pointer to the com_ptr.

			\code
				com_ptr<IFoo> foo;
				foo = auto_attach( raw_foo_pointer );
			\endcode
		*/
		com_ptr& operator=(const impl::auto_attach_t<interface_pointer>& p) throw()
		{ release(); ptr_ = p.get(); return *this; }

	private:
		template<typename Itf2>
		int compare_unknown(const com_ptr<Itf2>& x) const throw(com_error)
		{
			if ( (void *)ptr_ ==  (void *)x.get()) return 0;
			::IUnknown* p1 = 0;
			::IUnknown* p2 = 0;

			if (ptr_ != NULL) {
				Unknown_caller_::QueryInterface(ptr_,uuidof< ::IUnknown >(), reinterpret_cast<void**>(&p1)) | raise_exception;
				p1->Release();
			}

			if (x.get()) {
				Unknown_caller_::QueryInterface(x.get(), uuidof< ::IUnknown >(), reinterpret_cast<void**>(&p2)) | raise_exception;
				p2->Release();
			}

			return p1 - p2;
		}
		int compare_unknown(const identity_ptr& x) const throw(com_error)
		{
			if (ptr_ == x.get()) return 0;
			::IUnknown* p1 = 0;
            ::IUnknown* p2 = 0;

			if (ptr_ != NULL) {
				Unknown_caller_::QueryInterface(x.get(), uuidof< ::IUnknown >(), reinterpret_cast<void**>(&p2)) | raise_exception;
				p2->Release();
			}

			return p1 - p2;
		}
	public:
		/**! \name Pointer Comparison
		  */
		//@{
		template<typename T> bool operator<(const T& x) const throw(com_error)
		{ return compare_unknown(x) < 0; }

		template<typename T> bool operator>(const T& x) const throw(com_error)
		{ return compare_unknown(x) > 0; }

		template<typename T> bool operator<=(const T& x) const throw(com_error)
		{ return compare_unknown(x) <= 0; }

		template<typename T> bool operator>=(const T& x) const throw(com_error)
		{ return compare_unknown(x) >= 0; }

		/** Object equality.
		 */
		template<typename Itf2> bool operator==(const com_ptr<Itf2>& x) const throw(com_error)
		{ return compare_unknown(x) == 0; }

		template<typename Itf2> bool operator!=(const com_ptr<Itf2>& x) const throw(com_error)
		{ return compare_unknown(x) != 0; }

		//@}

		//! Returns true if the wrapped pointer is null.
		bool is_null() const throw()
		{ return (ptr_ ==NULL); }

		//! Returns true if the wrapped pointer is null.
		bool operator!() const throw()
		{ return is_null(); }

		//! Null comparison
		/*!
			Only comparison with a value of zero is allowed. Non-zero values will result
			in E_POINTER (wrapped in com_error) being thrown.

			\exception com_error
		*/
		bool operator==(int null) const
		{ if (null != 0) raise_exception(E_POINTER); return is_null(); }

		//! Null comparison
		/*!
			Only comparison with a value of zero is allowed. Non-zero values will result
			in E_POINTER (wrapped in com_error) being thrown.

			\exception com_error
		*/
		bool operator!=(int null) const
		{ if (null != 0) raise_exception(E_POINTER); return !is_null();	}

		//! Pointer comparison
		/*!
		   Returns true if the two pointers are the same.
		  */
		template<typename Itf2> inline bool same_pointer( const com_ptr<Itf2> &x) const throw(com_error)
		{ return (IUnknown *) ptr_ ==  (IUnknown *)x.ptr_; }

		//! Swap operation
		/*!
			This method is very fast, since it does not call AddRef or Release.
		*/
		void swap(com_ptr& x) throw()
		{ std::swap(ptr_, x.ptr_); }

		//! Detaches ownership.
		/*!
			Detaches the pointer from the wrapper and returns it as raw pointer.

			This method is primarily for use by the interface wrapper code. You should very seldomly find use for it.
		*/
		interface_pointer detach() throw()
		{ interface_pointer t = ptr_; ptr_ = 0;	return t; }

		//! Detaches ownership
		/*!
		*/
		static interface_pointer detach(com_ptr& x) throw()
		{ return x.detach(); }

		/**! Create a const reference to a pointer without invoking reference-counting.
		  * Since neither constructor or destructor get called.
		  */
		static const com_ptr& create_const_reference(const interface_pointer& x)
		{ return *reinterpret_cast<const com_ptr*>(&x);	}

		/**! Create a reference to a pointer without invoking reference-counting.
		  * Since neither constructor or destructor get called.
		  */
		static com_ptr& create_reference(interface_pointer& x)
		{ return *reinterpret_cast<com_ptr*>(&x);	}

		//! \name Adapter methods.
		//@{

		//! [in] adapter
		/*!
			Used when calling raw interfaces that require an [in] IXXX * argument.

			\code
				com_ptr<IFoo> foo;
				HRESULT hr = pRawInterface->raw_Method(foo.in());
			\endcode

			Only use this wrapper when forced to deal with raw interface.
		*/
		interface_pointer in() const throw()
		{ return ptr_; }

		interface_pointer raw() const throw()
		{ return ptr_; }

		interface_pointer get() const throw()
		{ return ptr_; }

		//! [out] adapter.
		/*!
			Used when calling raw interfaces that require an [out] IXXX ** argument.

			\code
				com_ptr<IFoo> foo;
				HRESULT hr = pRawInterface->raw_MethodThatReturnsPtr(foo.out());
			\endcode

			Only use this wrapper when forced to deal with raw interface.
		*/
		interface_pointer* out() throw()
		{ release(); return &ptr_; }

		//! [in, out] adapter.
		/*!
			Used when calling raw interfaces that require an [in, out] IXXX ** argument.

			\code
				com_ptr<IFoo> foo;
				HRESULT hr = pRawInterface->raw_MethodThatChangesPtr(foo.inout());
			\endcode

			Only use this wrapper when forced to deal with raw interface.
		*/
		interface_pointer* inout() throw()
		{ return &ptr_; }

		//@}

	private:
		class bool_tester
		{ void operator delete(void*); };

	public:
		operator bool_tester*() const throw()
		{ if (is_null()) return 0; static bool_tester test;	return &test; }

	private:
	   safe_interface_pointer get_safe_ptr() const throw()
	   { return reinterpret_cast<safe_interface_pointer>(ptr_); }

	   inline void create_nothrow(const variant_t& v) throw();

	   inline void create(const variant_t& v) throw(com_error);

	   void create(const uuid_t& clsid, const com_ptr< ::IUnknown>& outer, DWORD dwClsContext = CLSCTX_ALL) throw(com_error)
	   { CoCreateInstance(clsid, outer.in(), dwClsContext, iid(), reinterpret_cast<void**>(&ptr_)) | raise_exception; }

	   void create(const wchar_t* clsidString, const com_ptr< ::IUnknown>& outer, DWORD dwClsContext = CLSCTX_ALL) throw(com_error)
	   {
		   if (clsidString == NULL) raise_exception(E_INVALIDARG);

		   CLSID clsid;

		   if (clsidString[0] == '{')
			   CLSIDFromString(const_cast<LPOLESTR>(clsidString), &clsid) | raise_exception;
		   else
			   CLSIDFromProgID(clsidString, &clsid) | raise_exception;

		   create(clsid, outer, dwClsContext);;
	   }

	   void create(const wchar_t* name, BIND_OPTS& bind_opts) throw(com_error)
	   { CoGetObject(name, &bind_opts, iid(), reinterpret_cast<void**>(&ptr_)) | raise_exception; }
	}; // class

	//! Comparison with null
	/*!
		Only comparison with a value of zero is allowed. Non-zero values will result in E_POINTER (wrapped in com_error) being thrown.

		\relates com_ptr
		\exception com_error
			Throws E_POINTER if a non-zero value is specified.
	*/
	template<typename Itf2> inline bool operator==(int null, const com_ptr<Itf2>& x) throw(com_error)
	{ if(null != 0) raise_exception(E_POINTER); return x.is_null(); }

	//! Comparison with null
	/*!
		Only comparison with a value of zero is allowed. Non-zero values will result in E_POINTER (wrapped in com_error) being thrown.

		\relates identity_ptr
		\exception com_error
			Throws E_POINTER if a non-zero value is specified.
	*/
	static inline bool operator==(int null, const identity_ptr& x) throw(com_error)
	{ if(null != 0) raise_exception(E_POINTER); return x.is_null(); }

	//! Comparison with null
	/*!
		Only comparison with a value of zero is allowed. Non-zero values will result in E_POINTER (wrapped in com_error) being thrown.

		\relates com_ptr
		\exception com_error
			Throws E_POINTER if a non-zero value is specified.
	*/
	template<typename Itf> inline bool operator!=(int null, const com_ptr<Itf>& x) throw(com_error)
	{ return x != null; }

	//! Comparison with null
	/*!
		Only comparison with a value of zero is allowed. Non-zero values will result in E_POINTER (wrapped in com_error) being thrown.

		\relates com_ptr
		\exception com_error
			Throws E_POINTER if a non-zero value is specified.
	*/
	inline static bool operator!=(int null, const identity_ptr& x) throw(com_error)
	{ if(null != 0) raise_exception(E_POINTER); return !x.is_null(); }

	namespace impl {
		template <typename Itf>
		class try_caster_t
		{
			com_ptr<Itf> ptr_;
			public:
				template <typename Itf2 >
				try_caster_t( const com_ptr<Itf2> &ptr2)
				: ptr_(try_cast(ptr2))
				{}

				const com_ptr<Itf> &get() const { return ptr_; }

		};
	}
	/*! Version of try_cast more to the style of \b dynamic_cast.
	  This is aimed at being used for casts that get used once in an environment with many interfaces.
	  \code
	 	com_ptr<IDomSession> session;
	 	com_ptr<IDomUser>   = try_cast( try_cast_ptr<ISession>(session)->User() );
	  \endcode
	  The impl::try_caster_t is implementation only - let the compiler do the
	  cast from com_ptr to impl::try_caster_t. Note that try_cast would still
	  be the preferred method for most casts.
	  \sa try_cast
	  \relates com_ptr
	 */
	template<typename Itf> inline com_ptr<Itf> try_cast_ptr( const impl::try_caster_t<Itf> &caster)
	{ return (com_ptr<Itf>)caster.get(); }

	//! Cast com_ptr.
	/*!
		Allows QueryInterface when casting to different com_ptr type.

		\code
			com_ptr<IFoo> foo;
			com_ptr<IBar> bar;
			try {
				bar = try_cast(foo);
				bar->DoTheThing();
			} catch (com_error&) {
				// Cast didn't work.
			}
		\endcode

	*/
	template<typename Itf> inline impl::try_cast_t<Itf> try_cast(const com_ptr<Itf>& t) { return impl::try_cast_t<Itf>(t.get()); }
	static inline impl::try_cast_t< ::IUnknown> try_cast(const identity_ptr& t) { return impl::try_cast_t< ::IUnknown>(t.get()); }
	template<typename Itf> inline impl::try_cast_t<Itf> try_cast(Itf* t) { return impl::try_cast_t<Itf>(t); }
	inline impl::try_cast_t<variant_t> try_cast(const variant_t& v) { return impl::try_cast_t<variant_t>(v); }
	//@}

} // namespace

#include <comet/variant.h>
#include <comet/error.h>

namespace comet {
	template<typename Itf>
	inline void com_ptr<Itf>::create_nothrow(const variant_t& v) throw()
	{
		ptr_ = 0;
		::IUnknown *pUnk;
		switch (v.get_vt()) {
		case VT_DISPATCH:
			pUnk = V_DISPATCH(&v.get());
			break;
		case VT_UNKNOWN:
			pUnk = V_UNKNOWN(&v.get());
			break;
		case VT_DISPATCH|VT_BYREF:
			pUnk = *V_DISPATCHREF(&v.get());
			break;
		case VT_UNKNOWN|VT_BYREF:
			pUnk = *V_UNKNOWNREF(&v.get());
			break;
		default:
			// don't try anything fancy.
			return;
		}
		if( pUnk ==NULL) return;
		Unknown_caller_::QueryInterface(pUnk,iid(), reinterpret_cast<void**>(&ptr_));
	}
	template<typename Itf>
	inline void com_ptr<Itf>::create(const variant_t& v) throw(com_error)
	{
		ptr_ = 0;
		::IUnknown *pUnk;
		switch (v.get_vt()) {
		case VT_DISPATCH:
			pUnk = V_DISPATCH(&v.get());
			break;
		case VT_UNKNOWN:
			pUnk = V_UNKNOWN(&v.get());
			break;
		case VT_DISPATCH|VT_BYREF:
			pUnk = *V_DISPATCHREF(&v.get());
			break;
		case VT_UNKNOWN|VT_BYREF:
			pUnk = *V_UNKNOWNREF(&v.get());
			break;
		case VT_EMPTY:
		case VT_NULL:
			pUnk = NULL;
			break;
		default:
			// don't try anything fancy.
			raise_exception( E_INVALIDARG );
		}
		if( pUnk ==NULL) return;
		Unknown_caller_::QueryInterface(pUnk,iid(), reinterpret_cast<void**>(&ptr_)) | raise_exception;
	}

	inline identity_ptr::identity_ptr(const impl::try_cast_t<variant_t>& v) throw(com_error)
		: ptr_(NULL)
	{
		VARIANT vv = v.get().get();
		::IUnknown *pUnk;
		switch (V_VT(&vv)) {
			case VT_DISPATCH:
				pUnk = V_DISPATCH(&vv);
				break;
			case VT_UNKNOWN:
				pUnk = V_UNKNOWN(&vv);
				break;
			case VT_DISPATCH|VT_BYREF:
				pUnk = *V_DISPATCHREF(&vv);
				break;
			case VT_UNKNOWN|VT_BYREF:
				pUnk = *V_UNKNOWNREF(&vv);
				break;
			case VT_EMPTY:
			case VT_NULL:
				pUnk = NULL;
				break;
			default:
				// don't try anything fancy.
				raise_exception( E_INVALIDARG );
		}
		if( pUnk ==NULL) return;
		pUnk->QueryInterface( IID_IUnknown, reinterpret_cast<void **>(&ptr_)) | raise_exception;
	}

	inline identity_ptr::identity_ptr(const impl::com_cast_t<variant_t>& v) throw(com_error)
		: ptr_(NULL)
	{
		VARIANT vv = v.get().get();
		::IUnknown *pUnk;
		switch (V_VT(&vv)) {
			case VT_DISPATCH:
				pUnk = V_DISPATCH(&vv);
				break;
			case VT_UNKNOWN:
				pUnk = V_UNKNOWN(&vv);
				break;
			case VT_DISPATCH|VT_BYREF:
				pUnk = *V_DISPATCHREF(&vv);
				break;
			case VT_UNKNOWN|VT_BYREF:
				pUnk = *V_UNKNOWNREF(&vv);
				break;
			default:
				// don't try anything fancy.
				return;
		}
		if( pUnk ==NULL) return;
		pUnk->QueryInterface( IID_IUnknown, reinterpret_cast<void **>(&ptr_));
	}
}

namespace boost {
	template<typename Itf> inline Itf* get_pointer(const comet::com_ptr<Itf>& sp)
	{
		return sp.raw();
	}
	static inline IUnknown* get_pointer(const comet::identity_ptr &sp)
	{
		return sp.raw();
	}
}

//! Macro that specialises std::swap for com_ptr.
#define COMET_SPECIALISE_STD_SWAP_ITF(Itf) namespace std { inline void swap(comet::com_ptr<Itf>& x, comet::com_ptr<Itf>& y) {x.swap(y); } }

#endif

