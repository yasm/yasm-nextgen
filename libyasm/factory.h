#ifndef FACTORY_HEADER_DEFINED
#define FACTORY_HEADER_DEFINED
///
/// @file libyasm/factory.h
/// @brief Generic abstract factory template.
///
/// @license
///  Copyright 2001, Jim Hyslop, all rights reserved.
///
///  This file may be freely used, modified, and distributed, provided that the
///  accompanying copyright notice remains intact.
///
///  See the article Conversations: Abstract Factory, Template Style in the
///  C++ Users Journal, June 2001, http://www.ddj.com/dept/cpp/184403786
///
/// The generic abstract factory template is an implementation of the
/// Abstract Class Factory pattern, as a template (see "Design Patterns:
/// Elements of Reusable Object-Oriented Software", E. Gamma, R. Helm,
/// R. Johnson, J. Vlissides, Addison Wesley [1995] )
///
/// To use the template, you need to provide a base class and (optionally)
/// a key class. The base class must provide:
///  - a unique identifier
///
/// The key class must be able to be used as a key in a std::map, i.e. it must
///   - implement copy and assignment semantics
///   - provide bool operator< () const; (strictly speaking, 
///     it must provide bool less<classIDKey>() )
/// Default is std::string.
///
/// Steps to using the factory:
///   - Create the base class and its derivatives
///   - Register each class in the factory by instantiating a 
///     registerInFactory<> template class - do this in one file only (the
///     class implementation file is the perfect place for this)
///   - create the object by calling create() and passing it the same
///     value used when you instantiated the registerInFactory object.
///
/// For example:
///   base header:
///   class Base { /* whatever (don't forget the virtual dtor! */ };
///
///   base implementation:
///   registerInFactory<Base, Base, std::string> registerBase("Base");
///
///   derived header:
///   class Derived : public Base { /* whatever */ };
///
///   derived implementation:
///   registerInFactory<Base, Derived, std::string> registerDer("Derived");
///
///   code that instantiates the classes:
///   std::auto_ptr<Base> newBase = genericFactory<Base>::instance().create("Base");
///   std::auto_ptr<Base> newDerived = genericFactory<Base>::instance().create("Derived");
///
/// New derivatives can be added without affecting the existing code.
///
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace ddj {

// Implemented using the Singleton pattern

typedef std::string defaultIDKeyType;

template <class manufacturedObj, typename classIDKey=defaultIDKeyType>
class genericFactory
{
    /// A BASE_CREATE_FN is a function that takes no parameters
    /// and returns an auto_ptr to a manufactuedObj.  Note that
    /// we use no parameters, but you could add them
    /// easily enough to allow overloaded ctors.
    typedef std::auto_ptr<manufacturedObj> (*BASE_CREATE_FN)();

    /// FN_REGISTRY is the registry of all the BASE_CREATE_FN
    /// pointers registered.  Functions are registered using the
    /// regCreateFn member function (see below).
    typedef std::map<classIDKey, BASE_CREATE_FN> FN_REGISTRY;
    FN_REGISTRY registry;

    // Singleton implementation - private ctor & copying, with
    // no implementation on the copying.
    genericFactory();
    genericFactory(const genericFactory&); // Not implemented
    genericFactory &operator=(const genericFactory&); // Not implemented
public:

    /// Singleton access.
    static genericFactory &instance();

    /// Classes derived from manufacturedObj call this function once
    /// per program to register the class ID key, and a pointer to
    /// the function that creates the class.
    void regCreateFn(const classIDKey&, BASE_CREATE_FN);

    /// Create a new class of the type specified by className.
    std::auto_ptr<manufacturedObj> create(const classIDKey& className) const;

    /// Return a list of classes that are registered.
    std::vector<classIDKey> getRegisteredClasses() const;

    /// Return true if the specific class is registered.
    bool isRegisteredClass(const classIDKey& className) const;
};

template <class ancestorType, class manufacturedObj, typename classIDKey=defaultIDKeyType>
class registerInFactory
{
public:
    static std::auto_ptr<ancestorType> createInstance()
    {
        return std::auto_ptr<ancestorType>(new manufacturedObj);
    }
    registerInFactory(const classIDKey &id)
    {
        genericFactory<ancestorType>::instance().regCreateFn(id, createInstance);
    }
};

////////////////////////////////////////////////////////////////////////
// Implementation details.  If no comments appear, then I presume
// the implementation is self-explanatory.

template <class manufacturedObj, typename classIDKey>
genericFactory<manufacturedObj, classIDKey>::genericFactory()
{
}

template <class manufacturedObj, typename classIDKey>
genericFactory<manufacturedObj, classIDKey> &
genericFactory<manufacturedObj, classIDKey>::instance()
{
    static genericFactory theInstance;
    return theInstance;
}

// Register the creation function.  This simply associates the classIDKey
// with the function used to create the class.  The return value is a dummy
// value, which is used to allow static initialization of the registry.
// See example implementations in base.cc and derived.cc
template <class manufacturedObj, typename classIDKey>
void
genericFactory<manufacturedObj, classIDKey>::regCreateFn(const classIDKey &clName, BASE_CREATE_FN func)
{
    registry[clName]=func;
}

// The create function simple looks up the class ID, and if it's in the list,
// the statement "(*i).second();" calls the function.
template <class manufacturedObj, typename classIDKey>
std::auto_ptr<manufacturedObj>
genericFactory<manufacturedObj, classIDKey>::create(const classIDKey &className) const
{
    std::auto_ptr<manufacturedObj> ret(0);
    typename FN_REGISTRY::const_iterator regEntry=registry.find(className);
    if (regEntry != registry.end()) {
        ret = (*regEntry).second();
    }
    return ret;
}

// Just return a list of the classIDKeys used.
template <class manufacturedObj, typename classIDKey>
std::vector<classIDKey>
genericFactory<manufacturedObj, classIDKey>::getRegisteredClasses() const
{
    std::vector<classIDKey> ret(registry.size());
    int count;
    typename FN_REGISTRY::const_iterator regEntry;
    for (count=0,
         regEntry=registry.begin();
         regEntry!=registry.end();
         ++regEntry, ++count)
    {
        ret[count]=(*regEntry).first;
    }
    return ret;
}

template <class manufacturedObj, typename classIDKey>
bool
genericFactory<manufacturedObj, classIDKey>::isRegisteredClass(const classIDKey& className) const
{
    return registry.find(className) != registry.end();
}

} // namespace ddj

namespace yasm {

template <typename T>
inline std::auto_ptr<T>
load_module(const std::string& keyword)
{
    return ddj::genericFactory<T>::instance().create(keyword);
}

template <typename T>
inline bool
is_module(const std::string& keyword)
{
    return ddj::genericFactory<T>::instance().isRegisteredClass(keyword);
}

} // namespace yasm

#endif
