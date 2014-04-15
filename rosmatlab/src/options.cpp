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

#include <rosmatlab/options.h>
#include <rosmatlab/log.h>
#include <rosmatlab/exception.h>

#include <mex.h>

#include <boost/algorithm/string.hpp>
#include <limits>

namespace rosmatlab {

template <typename T>
static T getScalar(const mxArray *value)
{
  if (value && mxGetNumberOfElements(value) == 1) {
    switch(mxGetClassID(value)) {
      case mxSINGLE_CLASS:
        return *static_cast<float *>(mxGetData(value));
      case mxDOUBLE_CLASS:
        return *static_cast<double *>(mxGetData(value));
      case mxINT8_CLASS:
        return *static_cast<int8_T *>(mxGetData(value));
      case mxUINT8_CLASS:
        return *static_cast<uint8_T *>(mxGetData(value));
      case mxINT16_CLASS:
        return *static_cast<int16_T *>(mxGetData(value));
      case mxUINT16_CLASS:
        return *static_cast<uint16_T *>(mxGetData(value));
      case mxINT32_CLASS:
        return *static_cast<int32_T *>(mxGetData(value));
      case mxUINT32_CLASS:
        return *static_cast<uint32_T *>(mxGetData(value));
      case mxINT64_CLASS:
        return *static_cast<int64_T *>(mxGetData(value));
      case mxUINT64_CLASS:
        return *static_cast<uint64_T *>(mxGetData(value));

      default: break;
    }
  }

  return std::numeric_limits<T>::quiet_NaN();
}

bool Options::isScalar(const mxArray *value)
{
  return value && mxGetNumberOfElements(value) == 1;
}

bool Options::isString(const mxArray *value)
{
  return value && mxIsChar(value);
}

std::string Options::getString(const mxArray *value)
{
  if (!isString(value)) return std::string();
  const std::size_t len = mxGetNumberOfElements(value);
  //char temp[len + 1];
  char *temp = new char[len+1];
  
  mxGetString(value, temp, sizeof(temp));
  std::string str_temp = std::string(temp);
  delete[] temp;
  return str_temp;
}

bool Options::isDoubleScalar(const mxArray *value)
{
  if (!isScalar(value)) return false;

  switch(mxGetClassID(value)) {
    case mxSINGLE_CLASS:
    case mxDOUBLE_CLASS:
      return true;
    default: break;
  }

  return false;
}

double Options::getDoubleScalar(const mxArray *value)
{
  return getScalar<double>(value);
}

bool Options::isIntegerScalar(const mxArray *value)
{
  if (!isScalar(value)) return false;

  switch(mxGetClassID(value)) {
    case mxINT8_CLASS:
    case mxUINT8_CLASS:
    case mxINT16_CLASS:
    case mxUINT16_CLASS:
    case mxINT32_CLASS:
    case mxUINT32_CLASS:
    case mxINT64_CLASS:
    case mxUINT64_CLASS:
      return true;

    default: break;
  }

  return false;
}

int Options::getIntegerScalar(const mxArray *value)
{
  return getScalar<int>(value);
}

bool Options::isLogicalScalar(const mxArray *value)
{
  return value && mxIsLogicalScalar(value);
}

bool Options::getLogicalScalar(const mxArray *value)
{
  if (isLogicalScalar(value)) return mxIsLogicalScalarTrue(value);
  if (isIntegerScalar(value)) return getIntegerScalar(value);
  if (isDoubleScalar(value))  return getDoubleScalar(value);
  return false;
}

Options::Options()
{
}

Options::Options(int nrhs, const mxArray *prhs[], bool lowerCaseKeys)
{
  init(nrhs, prhs, lowerCaseKeys);
}

Options::~Options()
{
}

void Options::init(int nrhs, const mxArray *prhs[], bool lowerCaseKeys)
{
  // initialize from struct
  if (nrhs == 1 && mxIsStruct(prhs[0])) {
    int n = mxGetNumberOfFields(prhs[0]);
    for(int i = 0; i < n; ++i) {
      std::string key = mxGetFieldNameByNumber(prhs[0], i);
      if (lowerCaseKeys) boost::algorithm::to_lower(key);
      set(key, mxGetFieldByNumber(prhs[0], 0, i));
    }
    return;
  }

  // initialize from cell
  if (nrhs == 1 && mxIsCell(prhs[0])) {
    std::vector<const mxArray *> cell(mxGetNumberOfElements(prhs[0]));
    for(int i = 0; i < cell.size(); ++i) {
      cell[i] = mxGetCell(prhs[0], i);
    }
    init(cell.size(), cell.data());
    return;
  }

  // get default option
  if (nrhs % 2 != 0) {
    set(std::string(), *prhs);
    nrhs--; prhs++;
  }

  // iterate through key/value pairs
  for(; nrhs > 0; nrhs -= 2, prhs += 2) {
    if (!isString(prhs[0])) continue;
    std::string key = getString(prhs[0]);
    if (lowerCaseKeys) boost::algorithm::to_lower(key);
    set(key, prhs[1]);
  }
}

void Options::merge(const Options& other)
{
  for(ArrayMap::const_iterator it = other.arrays_.begin(); it != other.arrays_.end(); ++it) {
//    mexPrintf("merged array field %s\n", it->first.c_str());
    arrays_[it->first] = it->second;
    used_.erase(it->first);
  }
  for(StringMap::const_iterator it = other.strings_.begin(); it != other.strings_.end(); ++it) {
//    mexPrintf("merged string field %s\n", it->first.c_str());
    strings_[it->first] = it->second;
    used_.erase(it->first);
  }
  for(DoubleMap::const_iterator it = other.doubles_.begin(); it != other.doubles_.end(); ++it) {
//    mexPrintf("merged double field %s\n", it->first.c_str());
    doubles_[it->first] = it->second;
    used_.erase(it->first);
  }
  for(IntegerMap::const_iterator it = other.integers_.begin(); it != other.integers_.end(); ++it) {
//    mexPrintf("merged integer field %s\n", it->first.c_str());
    integers_[it->first] = it->second;
    used_.erase(it->first);
  }
  for(BoolMap::const_iterator it = other.bools_.begin(); it != other.bools_.end(); ++it) {
//    mexPrintf("merged bool field %s\n", it->first.c_str());
    bools_[it->first] = it->second;
    used_.erase(it->first);
  }
}

void Options::clear()
{
  arrays_.clear();
  strings_.clear();
  doubles_.clear();
  integers_.clear();
  bools_.clear();
  used_.clear();
}

bool Options::hasKey(const std::string& key) const
{
  return arrays_.count(key) || strings_.count(key) || doubles_.count(key) || integers_.count(key) || bools_.count(key);
}

const mxArray *Options::getArray(const std::string &key) const
{
  if (arrays_.count(key)) {
    used_.insert(key);
    return arrays_.at(key);
  }
  return 0;
}

const std::string& Options::getString(const std::string& key, const std::string& default_value) const
{
  if (strings_.count(key) && strings_.at(key).size()) {
    used_.insert(key);
    return strings_.at(key).front();
  }
  return default_value;
}

const Options::Strings& Options::getStrings(const std::string &key) const
{
  if (strings_.count(key)) {
    used_.insert(key);
    return strings_.at(key);
  }

  static const Strings empty;
  return empty;
}

double Options::getDouble(const std::string& key, double default_value) const
{
  if (doubles_.count(key) && doubles_.at(key).size()) {
    used_.insert(key);
    return doubles_.at(key).front();
  }
  return default_value;
}

const Options::Doubles& Options::getDoubles(const std::string &key) const
{
  if (doubles_.count(key)) {
    used_.insert(key);
    return doubles_.at(key);
  }
  static const Doubles empty;
  return empty;
}

int Options::getInteger(const std::string& key, int default_value) const
{
  if (integers_.count(key) && integers_.at(key).size()) {
    used_.insert(key);
    return integers_.at(key).front();
  }
  if (doubles_.count(key) && doubles_.at(key).size()) {
    used_.insert(key);
    return doubles_.at(key).front();
  }
  return default_value;
}

const Options::Integers& Options::getIntegers(const std::string &key) const
{
  if (integers_.count(key)) {
    used_.insert(key);
    return integers_.at(key);
  }
  static const Integers empty;
  return empty;
}

bool Options::getBool(const std::string& key, bool default_value) const
{
  if (bools_.count(key)   && bools_.at(key).size())   {
    used_.insert(key);
    return bools_.at(key).front();
  }
  if (doubles_.count(key) && doubles_.at(key).size()) {
    used_.insert(key);
    return doubles_.at(key).front();
  }
  return default_value;
}

const Options::Bools& Options::getBools(const std::string &key) const
{
  if (bools_.count(key)) {
    used_.insert(key);
    return bools_.at(key);
  }

  static const Bools empty;
  return empty;
}

Options &Options::set(const std::string& key, const std::string& value)
{
  strings_[key] = Strings(1, value);
  return *this;
}

Options &Options::set(const std::string& key, double value)
{
  doubles_[key] = Doubles(1, value);
  return *this;
}

Options &Options::set(const std::string& key, int value)
{
  integers_[key] = Integers(1, value);
  return *this;
}

Options &Options::set(const std::string& key, bool value)
{
  bools_[key] = Bools(1, value);
  return *this;
}

Options &Options::add(const std::string& key, const std::string& value)
{
  strings_[key].push_back(value);
  return *this;
}

Options &Options::add(const std::string& key, double value)
{
  doubles_[key].push_back(value);
  return *this;
}

Options &Options::add(const std::string& key, int value)
{
  integers_[key].push_back(value);
  return *this;
}

Options &Options::add(const std::string& key, bool value)
{
  bools_[key].push_back(value);
  return *this;
}

Options &Options::set(const std::string &key, const mxArray *value)
{
  arrays_[key] = value;

  if (isString(value)) {
    std::string str = getString(value);
    add(key, str);

    if (boost::algorithm::iequals(str, "on"))  add(key, true);
    if (boost::algorithm::iequals(str, "off")) add(key, false);
  }

  if (isDoubleScalar(value))  add(key, getDoubleScalar(value));
  if (isIntegerScalar(value)) add(key, getIntegerScalar(value));
  if (isLogicalScalar(value)) add(key, getLogicalScalar(value));

  if (mxIsCell(value)) {
    std::size_t n = mxGetNumberOfElements(value);
    for(mwIndex i = 0; i < n; ++i) {
      set(key, mxGetCell(value, i));
    }
  }
  return *this;
}

void Options::warnUnused() const
{
  for(StringMap::const_iterator it = strings_.begin(); it != strings_.end(); ++it) {
    if (!used_.count(it->first)) ROSMATLAB_PRINTF("WARNING: unused string argument '%s'", it->first.c_str());
  }
  for(DoubleMap::const_iterator it = doubles_.begin(); it != doubles_.end(); ++it) {
    if (!used_.count(it->first)) ROSMATLAB_PRINTF("WARNING: unused double argument '%s'", it->first.c_str());
  }
  for(IntegerMap::const_iterator it = integers_.begin(); it != integers_.end(); ++it) {
    if (!used_.count(it->first)) ROSMATLAB_PRINTF("WARNING: unused integer argument '%s'", it->first.c_str());
  }
  for(BoolMap::const_iterator it = bools_.begin(); it != bools_.end(); ++it) {
    if (!used_.count(it->first)) ROSMATLAB_PRINTF("WARNING: unused logical argument '%s'", it->first.c_str());
  }
  for(ArrayMap::const_iterator it = arrays_.begin(); it != arrays_.end(); ++it) {
    if (!used_.count(it->first)) ROSMATLAB_PRINTF("WARNING: unused argument '%s'", it->first.c_str());
  }
}

void Options::throwOnUnused() const
{
  for(StringMap::const_iterator it = strings_.begin(); it != strings_.end(); ++it) {
    if (!used_.count(it->first)) throw Exception("unknown string argument '" + it->first + "'");
  }
  for(DoubleMap::const_iterator it = doubles_.begin(); it != doubles_.end(); ++it) {
    if (!used_.count(it->first)) throw Exception("unknown double argument '" + it->first + "'");
  }
  for(IntegerMap::const_iterator it = integers_.begin(); it != integers_.end(); ++it) {
    if (!used_.count(it->first)) throw Exception("unknown integer argument '" + it->first + "'");
  }
  for(BoolMap::const_iterator it = bools_.begin(); it != bools_.end(); ++it) {
    if (!used_.count(it->first)) throw Exception("unknown logical argument '" + it->first + "'");
  }
  for(ArrayMap::const_iterator it = arrays_.begin(); it != arrays_.end(); ++it) {
    if (!used_.count(it->first)) throw Exception("unknown argument '" + it->first + "'");
  }
}

} // namespace rosmatlab
