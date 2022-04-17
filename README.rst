============
Charm-Kernel
============
An early-stage attempt to port the TMP-based chare/entry method registration scheme of `CharmLite <https://github.com/UIUC-PPL/charmlite>`_ (and, to an extent, `VT <https://github.com/DARMA-tasking/vt>`_) to Charm++.


Chares
======
NAME supports all the chare-types through the :code:`ck::chare` class template. Its first parameter is the user's class (CRTP), and its second is the chare-type, i.e., :code:`ck::main_chare`, :code:`ck::singleton_chare`, :code:`ck::group`, :code:`ck::nodegroup`,  or :code:`ck::array<Index>`. Note that :code:`ck::array` is itself a class template whose parameter is the index-type of the chare array. All the built-in indices are supported by default (i.e., :code:`CkIndex[1-6]D`), with user-defined index-types requiring specializations of the :code:`ck::index_view` class template (to define their encoding scheme). As with "conventional" Charm++, migratable chare-types must have migration constructors and :code:`pup` routines.

Entry Methods
=============
Entry methods are specified at the site of a call to :code:`send` or :code:`ck::make_callback`. They must be accessible members of chares whose arguments are de/serializable. Most attributes are supported through the :code:`CK_[ATTRIBUTE]_ENTRY` macro. For example, :code:`CK_THREADED_ENTRY(&foo:bar)` will define a threaded entry method. Note that these macros must be used at the top-level scope since they define specializations of class templates within the :code:`ck` namespace. NAME does not include a :code:`[reductiontarget]` attribute since its marshaled entry methods support multiple message-types by default.

Proxies
=======
All singleton chares (and main chares, by extension) use :code:`ck::chare_proxy` proxies. All other chare-types have :code:`ck:collection_proxy`, :code:`ck::element_proxy`, and :code:`ck::section_proxy` proxies. All proxies have a :code:`send` method that broadcasts, multicast, or sends a message to the chares they encompass. Calls to :code:`send` can, optionally, include :code:`CkEntryOptions*` to specify message priorities, queuing strategies, etc. Non-element proxies have static :code:`create` methods to create new instances, while element proxies have an :code:`insert` routine that functions similarly. 

Callbacks
=========
Users can create callbacks to entry methods using the `ck::make_callback` function template. Its template parameter is an entry method, while its argument is the target proxy.