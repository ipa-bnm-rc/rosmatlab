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

#include <mex.h>
#include <rosmatlab/ros.h>
#include <rosmatlab/options.h>

using namespace rosmatlab;

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  if (nrhs < 1) {
    throw Exception("At least 1 input argument is required");
  }

  try {
    init();
    Publisher *publisher = getObject<Publisher>(*prhs++); nrhs--;
    std::string method;
    if (nrhs) { method = Options::getString(*prhs++); nrhs--; }

    // construction
    if (method == "create") {
      delete publisher;
      mexPrintf("[rosmatlab] Creating new Publisher object\n");
      publisher = new Publisher(nrhs, prhs);
      plhs[0] = publisher->handle();
      return;
    }

    // destruction
    if (method == "delete") {
      mexPrintf("[rosmatlab] Deleting Publisher object\n");
      delete publisher;
      return;
    }

    if (!publisher) {
      throw Exception("Publisher instance not found");
    }

    // subscribe()
    if (method == "advertise") {
      plhs[0] = mxCreateLogicalScalar(publisher->advertise(nrhs, prhs));
      return;
    }

    // publish()
    if (method == "publish") {
      publisher->publish(nrhs, prhs);
      return;
    }

    // getTopic()
    if (method == "getTopic") {
      plhs[0] = publisher->getTopic();
      return;
    }

    // getNumSubscribers()
    if (method == "getNumSubscribers") {
      plhs[0] = publisher->getNumSubscribers();
      return;
    }

    // isLatched()
    if (method == "isLatched") {
      plhs[0] = publisher->isLatched();
      return;
    }

    // unknown method exception
    throw Exception("unknown method '" + method + "'");

  } catch(Exception &e) {
    mexErrMsgTxt(e.what());
  }
}
