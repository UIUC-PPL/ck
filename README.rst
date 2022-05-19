============
Charm-Kernel
============
An early-stage attempt to port the TMP-based chare/entry method registration scheme of `CharmLite <https://github.com/UIUC-PPL/charmlite>`_ (and, to an extent, `VT <https://github.com/DARMA-tasking/vt>`_) to Charm++.

Other links:

* `Project Proposal+Status Document. <https://docs.google.com/document/d/1wlwCiCTgDlMPrD47PAcg5M_f42R4o37ykFi8IzUpDvE/edit?usp=sharing>`_

* `Project Feedback Collection Poll. <https://forms.gle/XLwDVLuJ8skbWsL48>`_

Building+Running
================
Currently NAME is built out-of-tree with Charm++, requiring no modifications to the mainline repository. To build a standalone program, include the :code:`include/` directory in this repo, and add the flags to enable C++17 to :code:`charmc` (i.e., :code:`-c++-option -std=c++17`). The simplest way to build and run all of NAME's test programs is to run the :code:`tests/run.sh` script.

Chares
======
NAME supports all the chare-types through the :code:`ck::chare` class template. Its first parameter is the user's class (CRTP), and its second is the chare-type, i.e., :code:`ck::main_chare`, :code:`ck::singleton_chare`, :code:`ck::group`, :code:`ck::nodegroup`,  or :code:`ck::array<Index>`. Note that :code:`ck::array` is itself a class template whose parameter is the index-type of the chare array. All the built-in indices are supported by default (i.e., :code:`CkIndex[1-6]D`), with user-defined index-types requiring specializations of the :code:`ck::index_view` class template (to define their encoding scheme). As with "conventional" Charm++, migratable chare-types must have migration constructors and :code:`pup` routines.

Entry Methods
=============
Entry methods are specified at the site of a call to :code:`ck::send` or :code:`ck::make_callback`. They must be accessible members of chares whose arguments are de/serializable. Most attributes are supported through the :code:`CK_[ATTRIBUTE]_ENTRY` macro. For example, :code:`CK_THREADED_ENTRY(&foo:bar)` will define a threaded entry method. Note that these macros must be used at the top-level scope since they define specializations of class templates within the :code:`ck` namespace. NAME does not include a :code:`[reductiontarget]` attribute since its marshaled entry methods support multiple message-types by default.

Call-site attribution
---------------------
One may assign certain attributes at the site of a :code:`ck::send` method. For example, one may try to execute an entry method inline via:

.. code-block:: cpp
    
    // a single attribute may be passed directly
    ck::send<&foo::bar, ck::Inline>(fooProxy, ...);
    // multiple attributes must be wrapped in the `ck::attributes` helper
    ck::send<&foo::bar, ck::attributes<ck::Inline, ...>>(fooProxy, ...); 

Advanced Attribution
--------------------
Assigning an attribute to all specializations of a generic entry method is non-trivial, requiring use of SFINAE. Consider the following:

.. code-block:: cpp

    class main : public ck::chare<main, ck::main_chare> {
    /* ... */
    // an entry method that we want to make [inline]
    template <typename A, typename B>
    void receive(const A& a, const B& b) {
        /* ... */
    }
    };
    // required to pass instances via template parameters
    template <typename A, typename B>
    using receive_like_t = void (main::*)(const A&, const B&);
    // ONE CAN EITHER MANUALLY WRITE THE SPECIALIZATION
    namespace ck {
    template <typename A, typename B, receive_like_t<A, B> RL>
    struct is_inline<RL, std::enable_if_t<(RL == &main::receive<A, B>)>>
        : public std::true_type {};
    }
    // OR USE THE CK_ENTRY_ASSIGN_ATTRIBUTE MACRO TO DO IT
    CK_ENTRY_ASSIGN_ATTRIBUTE((RL, std::enable_if_t<(RL == &main::receive<A, B>)>),
                            inline, typename A, typename B,
                            receive_like_t<A, B> RL);
    // EITHER WAY WILL GET THE JOB DONE
    static_assert(ck::is_inline_v<&main::receive<int, int>>);

Pointer-to-offset Optimizations
-------------------------------
Instead of copying the data from the message into another buffer, e.g., as with a :code:`std::vector`, :code:`ck::span` containers can directly reference and retain (potentially shared) ownership of a message buffer. This process avoids receiver-side copies, and it is referred to as a pointer-to-offset optimization (i.e., it uses an offset within the message buffer as an array/pointer). Note, NAME only applies these optimizations to :code:`ck::span` containers of bytes-like types received via parameter marshalling.

Proxies
=======
All singleton chares (and main chares, by extension) use :code:`ck::chare_proxy` proxies. All other chare-types have :code:`ck:collection_proxy`, :code:`ck::element_proxy`, and :code:`ck::section_proxy` proxies. All proxies use the :code:`ck::send` function, which broadcasts, multicast, or sends a message to the chare(s) they encompass. Calls to :code:`ck::send` can, optionally, include :code:`CkEntryOptions*` to specify message priorities, queuing strategies, etc. Non-element proxies use the :code:`ck::create` function to create new instances. Chare-array element proxies can use the :code:`ck::insert` function, which behaves similarly (although it takes a proxy as its first argument). 

Callbacks
=========
Users can create callbacks to entry methods using the :code:`ck::make_callback` function template. Its template parameter is an entry method, while its argument is the target proxy.

Reducers
========
One can register custom reducer functions (i.e., those that combine contributions during reductions) using the :code:`ck::reducer` function. Generally, valid reducers are of the form :code:`T(*)(T,T)`, although their arguments may be :code:`const` or r-value references. Note that we consider all reducers over :code:`PUPBytes` types as "streamable." We provide a helper function for forming contributions, :code:`pack_contribution` (name pending).

Readonly Variables
==================
NAME supports defining readonly variables via the :code:`CK_READONLY` macro. One can forward declare readonly variables using the :code:`CK_EXTERN_READONLY` macro. Note that types of readonly variables containing commas must be enclosed within parenthesis. For example, :code:`CK_READONLY((std::map<int, int>), kReadonlyMap)` is a valid declaration. 
