//=================================================================================================
// Copyright (c) 2012, Johannes Meyer, TU Darmstadt
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the Flight Systems and Automatic Control group,
//       TU Darmstadt, nor the names of its contributors may be used to
//       endorse or promote products derived from this software without
//       specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//=================================================================================================

#ifndef ROSMATLAB_OBJECT_H
#define ROSMATLAB_OBJECT_H

#include <rosmatlab/exception.h>
#include <rosmatlab/options.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/type_traits.hpp>

#include "mex.h"

#include <stdint.h>

namespace rosmatlab {

namespace {
  struct null_deleter {
    void operator()(void const *) const {}
  };
}

template <class Type>
class Object
{
public:
  typedef boost::shared_ptr<Type> Ptr;

  Object() { construct(); }
  Object(Type *instance) { *this = instance; construct(); }
  Object(const Ptr &instance) { *this = instance; construct(); }
  Object(const Object &other) { *this = other; construct(); }
  virtual ~Object() { if (handle_) mxDestroyArray(handle_); }

  const Ptr &instance() const { return instance_; }
  mxArray *handle() const { return handle_; }

  Type* get() { return instance_.get(); }
  const Type* get() const { return instance_.get(); }
  Type &operator*() { return *instance_; }
  const Type &operator*() const { return *instance_; }

  Object<Type> &operator=(const Object &other) {
    return *this = other.instance_;
  }

  Object<Type> &operator=(Type *instance) {
    return *this = Ptr(instance, null_deleter());
  }

  Object<Type> &operator=(const Ptr &instance) {
    instance_ = instance;
    return *this;
  }

  static Object<Type> *byHandle(const mxArray *handle) {
    const mxArray *ptr = 0;
    if (!handle) return 0;
    // mexPrintf(ROSMATLAB_PRINTF_PREFIX "Searching for object of type %s (%s)...\n", getClassName(), typeid(Type).name());
    if (mxIsClass(handle, class_name_)) {
      // mexPrintf(ROSMATLAB_PRINTF_PREFIX "Handle is a %s class\n", mxGetClassName(handle));
      ptr = mxGetProperty(handle, 0, "handle");
    } else if (mxIsStruct(handle)) {
      // mexPrintf(ROSMATLAB_PRINTF_PREFIX "Handle is a struct\n");
      ptr = mxGetField(handle, 0, "handle");
    } else if (mxIsDouble(handle)) {
      // mexPrintf(ROSMATLAB_PRINTF_PREFIX "Handle is a double\n");
      ptr = handle;
    }
    if (!ptr || !mxIsDouble(ptr) || !(mxGetNumberOfElements(ptr) > 0) || !mxGetPr(ptr)) throw Exception("invalid handle");

    Object<Type> *object = reinterpret_cast<Object<Type> *>(static_cast<uint64_t>(*mxGetPr(ptr)));
    return object;
  }

  static const char *getClassName() { return class_name_; }

private:
  boost::shared_ptr<Type> instance_;
  mxArray *handle_;
  static const char *class_name_;

  void construct() {
    handle_ = mxCreateDoubleScalar(reinterpret_cast<uint64_t>(this));
    mexMakeArrayPersistent(handle_);
    assert(byHandle(handle()) == this);
  }
};

template <class Type>
Type *getObject(const mxArray *handle) {
  Object<Type> *object = Object<Type>::byHandle(handle);
  if (!object) return 0;
  return object->get();
}

namespace internal {

  template <typename T> static inline mxArray *mx_cast(T result) {
    return mxCreateDoubleScalar(result);
  }

  template<> inline mxArray *mx_cast(mxArray *result) {
    return result;
  }

  template<> inline mxArray *mx_cast(bool result) {
    return mxCreateLogicalScalar(result);
  }

  template<> inline mxArray *mx_cast(const std::string& result) {
    return mxCreateString(result.c_str());
  }

  template <class Type, class Result>
  static inline void callMex(const boost::function<Result(Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])> &func, Type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    plhs[0] = mx_cast(func(obj, nlhs, plhs, nrhs, prhs));
  }

  template <class Type, class Result>
  static inline void callMex(const boost::function<Result(Type *, int nrhs, const mxArray *prhs[])> &func, Type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    plhs[0] = mx_cast(func(obj, nrhs, prhs));
  }

  template <class Type, class Result>
  static inline void callMex(const boost::function<Result(Type *)> &func, Type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    plhs[0] = mx_cast(func(obj));
  }

  template <class Type>
  static inline void callMex(const boost::function<void(Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])> &func, Type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    func(obj, nlhs, plhs, nrhs, prhs);
  }

  template <class Type>
  static inline void callMex(const boost::function<void(Type *, int nrhs, const mxArray *prhs[])> &func, Type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    func(obj, nrhs, prhs);
  }

  template <class Type>
  static inline void callMex(const boost::function<void(Type *)> &func, Type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
    func(obj);
  }

  template <class Signature>
  class MexFunctor {
  public:
    typedef Signature* function_pointer;
    typedef boost::function<Signature> function_type;
    typedef typename boost::remove_pointer<typename boost::function_traits<Signature>::arg1_type>::type object_type;

    MexFunctor(const function_type& func) : target_(func) {}
    MexFunctor(function_pointer func) : target_(func) {}

    void operator()(object_type *obj, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) {
      callMex(target_, obj, nlhs, plhs, nrhs, prhs);
    }

  private:
    boost::function<Signature> target_;
  };
}

template <class Type>
class MexMethodMap {
private:
//  typedef boost::function<void(Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])> FunctionType1;
//  typedef boost::function<void(Type *, int nrhs, const mxArray *prhs[])> FunctionType2;
//  typedef boost::function<void(Type *)> FunctionType3;
//  typedef boost::function<mxArray *(Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])> FunctionType4;
//  typedef boost::function<mxArray *(Type *, int nrhs, const mxArray *prhs[])> FunctionType5;
//  typedef boost::function<mxArray *(Type *)> FunctionType6;

//  std::map<std::string,FunctionType1> methods1_;
//  std::map<std::string,FunctionType2> methods2_;
//  std::map<std::string,FunctionType3> methods3_;
//  std::map<std::string,FunctionType4> methods4_;
//  std::map<std::string,FunctionType5> methods5_;
//  std::map<std::string,FunctionType6> methods6_;

  std::map<std::string,boost::function<void(Type *, int, mxArray *[], int, const mxArray *[])> > methods_;

  bool initialized_;
  bool throw_on_unknown_;

public:
  MexMethodMap() : initialized_(false), throw_on_unknown_(false) {}

  bool initialize() {
    if (!initialized_) {
      initialized_ = true;
      return false;
    }
    return true;
  }

  operator void *() const {
    return initialized_;
  }

  MexMethodMap &throwOnUnknown(bool value = true) {
    throw_on_unknown_ = value;
    return *this;
  }

  template <typename Result>
  MexMethodMap &add(const std::string& name, Result (Type::*function)(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])) {
    methods_[name] = internal::MexFunctor<Result (Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])>(function);
    return *this;
  }

  template <typename Result>
  MexMethodMap &add(const std::string& name, Result (Type::*function)(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) const) {
    methods_[name] = internal::MexFunctor<Result (const Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])>(function);
    return *this;
  }

  template <typename Result>
  MexMethodMap &add(const std::string& name, Result (Type::*function)(int nrhs, const mxArray *prhs[])) {
    methods_[name] = internal::MexFunctor<Result (Type *, int nrhs, const mxArray *prhs[])>(function);
    return *this;
  }

  template <typename Result>
  MexMethodMap &add(const std::string& name, Result (Type::*function)(int nrhs, const mxArray *prhs[]) const) {
    methods_[name] = internal::MexFunctor<Result (const Type *, int nrhs, const mxArray *prhs[])>(function);
    return *this;
  }

  template <typename Result>
  MexMethodMap &add(const std::string& name, Result (Type::*function)()) {
    methods_[name] = internal::MexFunctor<Result (Type *)>(function);
    return *this;
  }

  template <typename Result>
  MexMethodMap &add(const std::string& name, Result (Type::*function)() const) {
    methods_[name] = internal::MexFunctor<Result (const Type *)>(function);
    return *this;
  }

//  MexMethodMap &add(const std::string& name, mxArray *(Type::*function)(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])) {
//    methods_[name] = internal::MexFunctor<mxArray *(Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])>(function);
//    return *this;
//  }

//  MexMethodMap &add(const std::string& name, mxArray *(Type::*function)(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]) const) {
//    methods_[name] = internal::MexFunctor<mxArray *(const Type *, int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])>(function);
//    return *this;
//  }

//  MexMethodMap &add(const std::string& name, mxArray *(Type::*function)(int nrhs, const mxArray *prhs[])) {
//    methods_[name] = internal::MexFunctor<mxArray *(Type *, int nrhs, const mxArray *prhs[])>(function);
//    return *this;
//  }

//  MexMethodMap &add(const std::string& name, mxArray *(Type::*function)(int nrhs, const mxArray *prhs[]) const) {
//    methods_[name] = internal::MexFunctor<mxArray *(const Type *, int nrhs, const mxArray *prhs[])>(function);
//    return *this;
//  }

//  MexMethodMap &add(const std::string& name, mxArray *(Type::*function)()) {
//    methods_[name] = internal::MexFunctor<mxArray *(Type *)>(function);
//    return *this;
//  }

//  MexMethodMap &add(const std::string& name, mxArray *(Type::*function)() const) {
//    methods_[name] = internal::MexFunctor<mxArray *(const Type *)>(function);
//    return *this;
//  }

  bool has(const std::string& name) const {
    return methods_.count(name);
  }

  bool call(Type *object, const std::string& name, int &nlhs, mxArray **&plhs, int &nrhs, const mxArray **&prhs) const {
    if (methods_.count(name)) {
      methods_.at(name)(object, nlhs, plhs, nrhs, prhs);
      return true;
    }

    if (throw_on_unknown_)
      throw Exception(std::string() + "unknown method '" + name + "' for objects of class " + Object<Type>::getClassName());

    if (has("default")) {
      return call(object, "default", nlhs, plhs, nrhs, prhs);
    }

    return false;
  }
};

template <class Type>
Type *mexClassHelper(int &nlhs, mxArray **&plhs, int &nrhs, const mxArray **&prhs, std::string& method, const MexMethodMap<Type>& methods = MexMethodMap<Type>()) {
  if (nrhs < 1) {
    throw ArgumentException(1);
  }

  Type *object = getObject<Type>(*prhs++); nrhs--;
  method.clear();
  if (nrhs) { method = Options::getString(*prhs++); nrhs--; }

  // construction
  if (method == "create") {
    delete object;
    // mexPrintf(ROSMATLAB_PRINTF_PREFIX "Creating new %s object\n", Object<Type>::getClassName().c_str());
    object = new Type(nrhs, prhs);
    plhs[0] = object->handle();
    method.clear();
    return object;
  }

  // destruction
  if (method == "delete") {
    // mexPrintf(ROSMATLAB_PRINTF_PREFIX "Deleting %s object\n", Object<Type>::getClassName().c_str());
    delete object;
    return 0;
  }

  // for all other methods the object must exist
  if (!object) {
    throw Exception("Instance not found");
  }

  // execute method
  if (methods.call(object, method, nlhs, plhs, nrhs, prhs)) {
    method.clear();
  }

  return object;
}

template <class Type>
Type *mexClassHelper(int &nlhs, mxArray **&plhs, int &nrhs, const mxArray **&prhs, const MexMethodMap<Type>& methods = MexMethodMap<Type>()) {
  std::string method;
  return mexClassHelper(nlhs, plhs, nrhs, prhs, method, methods);
}

} // namespace rosmatlab

#endif // ROSMATLAB_OBJECT_H
