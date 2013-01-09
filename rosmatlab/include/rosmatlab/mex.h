//=================================================================================================
// Copyright (c) 2013, Johannes Meyer, TU Darmstadt
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

#ifndef ROSMATLAB_MEX_H
#define ROSMATLAB_MEX_H

#include <mex.h>
#include <iostream>

#include <rosmatlab/exception.h>

namespace rosmatlab {

class MexEnvironment {
private:
  class matlab_streambuf : public std::streambuf {
  public:
  protected:
    virtual std::streamsize xsputn(const char *s, std::streamsize n) {
      mexPrintf("%.*s",n,s);
      return n;
    }

    virtual int overflow(int c = EOF) {
      if (c != EOF) {
        mexPrintf("%.1s",&c);
      }
      return 1;
    }
  };

public:
  MexEnvironment(int nrhs = 0, const mxArray **prhs = 0) {
    outbuf = std::cout.rdbuf(&mout);
  }

  ~MexEnvironment() {
    std::cout.rdbuf(outbuf);
  }

private:
  matlab_streambuf mout;
  std::streambuf *outbuf;
};

} // namespace rosmatlab

#endif // ROSMATLAB_MEX_H
