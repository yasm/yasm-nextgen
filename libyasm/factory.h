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
///   registerModule<Base, Derived> registerDer("Derived");
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

#include "module.h"


namespace yasm {

// standard yasm module types
class Arch;
class DebugFormat;
class ListFormat;
class ObjectFormat;
class Parser;
class Preprocessor;

// Implemented using the Singleton pattern

template <typename manufacturedObj>
class moduleFactory
{
    /// A BASE_CREATE_FN is a function that takes no parameters
    /// and returns an auto_ptr to a manufactuedObj.  Note that
    /// we use no parameters, but you could add them
    /// easily enough to allow overloaded ctors.
    typedef void* (*BASE_CREATE_FN)();

    /// FN_REGISTRY is the registry of all the BASE_CREATE_FN
    /// pointers registered.  Functions are registered using the
    /// regCreateFn member function (see below).
    typedef std::map<std::string, BASE_CREATE_FN> FN_REGISTRY;
    FN_REGISTRY registry;

    // Singleton implementation - private ctor & copying, with
    // no implementation on the copying.
    moduleFactory();
    moduleFactory(const moduleFactory&); // Not implemented
    moduleFactory &operator=(const moduleFactory&); // Not implemented
public:

    /// Singleton access.
    static moduleFactory &instance();

    /// Classes derived from manufacturedObj call this function once
    /// per program to register the class ID key, and a pointer to
    /// the function that creates the class.
    void regCreateFn(const std::string&, BASE_CREATE_FN);

    /// Create a new class of the type specified by className.
    std::auto_ptr<manufacturedObj> create(const std::string& className) const;

    std::auto_ptr<Module> createBase(const std::string& className) const;

    /// Return a list of classes that are registered.
    std::vector<std::string> getRegisteredClasses() const;

    /// Return true if the specific class is registered.
    bool isRegisteredClass(const std::string& className) const;
};

template <typename ancestorType, typename manufacturedObj>
class registerModule
{
private:
    static void* createInstance()
    {
        return new manufacturedObj;
    }

public:
    registerModule(const std::string& id)
    {
        moduleFactory<ancestorType>::instance().regCreateFn(id, createInstance);
    }
};

////////////////////////////////////////////////////////////////////////
// Implementation details.  If no comments appear, then I presume
// the implementation is self-explanatory.

template <typename manufacturedObj>
moduleFactory<manufacturedObj>::moduleFactory()
{
}

template <typename manufacturedObj>
moduleFactory<manufacturedObj> &
moduleFactory<manufacturedObj>::instance()
{
    static moduleFactory theInstance;
    return theInstance;
}

// Register the creation function.  This simply associates the classIDKey
// with the function used to create the class.  The return value is a dummy
// value, which is used to allow static initialization of the registry.
// See example implementations in base.cc and derived.cc
template <typename manufacturedObj>
void
moduleFactory<manufacturedObj>::regCreateFn(const std::string& clName,
                                            BASE_CREATE_FN func)
{
    registry[clName]=func;
}

// The create function simple looks up the class ID, and if it's in the list,
// the statement "(*i).second();" calls the function.
template <typename manufacturedObj>
std::auto_ptr<manufacturedObj>
moduleFactory<manufacturedObj>::create(const std::string& className) const
{
    std::auto_ptr<manufacturedObj> ret(0);
    typename FN_REGISTRY::const_iterator regEntry=registry.find(className);
    if (regEntry != registry.end()) {
        ret.reset(static_cast<manufacturedObj*>((*regEntry).second()));
    }
    return ret;
}

// createBase is similar to create, except it returns a base class instead
// of manufacturedObj.
template <typename manufacturedObj>
std::auto_ptr<Module>
moduleFactory<manufacturedObj>::createBase(const std::string& className) const
{
    std::auto_ptr<Module> ret(0);
    typename FN_REGISTRY::const_iterator regEntry=registry.find(className);
    if (regEntry != registry.end()) {
        ret.reset(static_cast<Module*>((*regEntry).second()));
    }
    return ret;
}

// Just return a list of the classIDKeys used.
template <typename manufacturedObj>
std::vector<std::string>
moduleFactory<manufacturedObj>::getRegisteredClasses() const
{
    std::vector<std::string> ret(registry.size());
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

template <typename manufacturedObj>
bool
moduleFactory<manufacturedObj>::isRegisteredClass(const std::string& className) const
{
    return registry.find(className) != registry.end();
}

template <typename T>
inline std::auto_ptr<T>
load_module(const std::string& keyword)
{
    return moduleFactory<T>::instance().create(keyword);
}

template <typename T>
inline bool
is_module(const std::string& keyword)
{
    return moduleFactory<T>::instance().isRegisteredClass(keyword);
}

} // namespace yasm

#endif
