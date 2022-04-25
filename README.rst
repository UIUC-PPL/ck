============
Charm-Kernel
============
An early-stage attempt to port the TMP-based chare/entry method registration scheme of `CharmLite <https://github.com/UIUC-PPL/charmlite>`_ (and, to an extent, `VT <https://github.com/DARMA-tasking/vt>`_) to Charm++.

Other links:

* `Project Proposal+Status Document. <https://docs.google.com/document/d/1wlwCiCTgDlMPrD47PAcg5M_f42R4o37ykFi8IzUpDvE/edit?usp=sharing>`_

* `Project Feedback Collection Poll. <https://forms.gle/XLwDVLuJ8skbWsL48>`_

Chares
======
NAME supports all the chare-types through the :code:`ck::chare` class template. Its first parameter is the user's class (CRTP), and its second is the chare-type, i.e., :code:`ck::main_chare`, :code:`ck::singleton_chare`, :code:`ck::group`, :code:`ck::nodegroup`,  or :code:`ck::array<Index>`. Note that :code:`ck::array` is itself a class template whose parameter is the index-type of the chare array. All the built-in indices are supported by default (i.e., :code:`CkIndex[1-6]D`), with user-defined index-types requiring specializations of the :code:`ck::index_view` class template (to define their encoding scheme). As with "conventional" Charm++, migratable chare-types must have migration constructors and :code:`pup` routines.

Entry Methods
=============
Entry methods are specified at the site of a call to :code:`send` or :code:`ck::make_callback`. They must be accessible members of chares whose arguments are de/serializable. Most attributes are supported through the :code:`CK_[ATTRIBUTE]_ENTRY` macro. For example, :code:`CK_THREADED_ENTRY(&foo:bar)` will define a threaded entry method. Note that these macros must be used at the top-level scope since they define specializations of class templates within the :code:`ck` namespace. NAME does not include a :code:`[reductiontarget]` attribute since its marshaled entry methods support multiple message-types by default.

Pointer-to-offset Optimizations
-------------------------------
Instead of copying the data from the message into another buffer, e.g., as with a :code:`std::vector`, :code:`ck::span` containers can directly reference and retain (potentially shared) ownership of a message buffer. This process avoids receiver-side copies, and it is referred to as a pointer-to-offset optimization (i.e., it uses an offset within the message buffer as an array/pointer). Note, NAME only applies these optimizations to :code:`ck::span` containers of bytes-like types received via parameter marshalling.

Proxies
=======
All singleton chares (and main chares, by extension) use :code:`ck::chare_proxy` proxies. All other chare-types have :code:`ck:collection_proxy`, :code:`ck::element_proxy`, and :code:`ck::section_proxy` proxies. All proxies have a :code:`send` method that broadcasts, multicast, or sends a message to the chares they encompass. Calls to :code:`send` can, optionally, include :code:`CkEntryOptions*` to specify message priorities, queuing strategies, etc. Non-element proxies have static :code:`create` methods to create new instances, while element proxies have an :code:`insert` routine that functions similarly. 

Callbacks
=========
Users can create callbacks to entry methods using the :code:`ck::make_callback` function template. Its template parameter is an entry method, while its argument is the target proxy.

Reducers
========
One can register custom reducer functions (i.e., those that combine contributions during reductions) using the :code:`ck::reducer` function. Generally, valid reducers are of the form :code:`T(*)(T,T)`, although their arguments may be :code:`const` or r-value references. Note that we consider all reducers over :code:`PUPBytes` types as "streamable." We provide a helper function for forming contributions, :code:`pack_contribution` (name pending).
