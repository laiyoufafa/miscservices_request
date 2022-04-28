/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "fail_callback.h"
#include "upload_task.h"

namespace OHOS::Request::Upload {
FailCallback::FailCallback(napi_env env, napi_value callback)
    : env_(env)
{
    napi_create_reference(env, callback, 1, &callback_);
    napi_get_uv_event_loop(env, &loop_);
}

FailCallback::~FailCallback()
{
    napi_delete_reference(env_, callback_);
    status_ = napi_generic_failure;
}

void FailCallback::CheckQueueWorkRet(int ret, FailWorker *failWorker, uv_work_t *work)
{
    if (ret != 0) {
        if (failWorker != nullptr) {
            delete failWorker;
            failWorker = nullptr;
        }
        if (work != nullptr) {
            delete work;
            work = nullptr;
        }
    }
}

void FailCallback::Fail(const unsigned int error)
{
    UPLOAD_HILOGD(UPLOAD_MODULE_JS_NAPI, "Fail. error : %{public}d", error);
    FailWorker *failWorker = new FailWorker(this, error);
    uv_work_t *work = new uv_work_t;
    work->data = failWorker;
    int ret = uv_queue_work(loop_, work,
        [](uv_work_t *work) {},
        [](uv_work_t *work, int status) {
            FailWorker *failWorkerInner = reinterpret_cast<FailWorker *>(work->data);
            napi_value jsError = nullptr;
            napi_value callback = nullptr;
            napi_value args[1];
            napi_value global = nullptr;
            napi_value result;
            napi_status callStatus = napi_generic_failure;
            napi_env tmpEnv = failWorkerInner->callback->env_;
            if (failWorkerInner->callback->env_ == tmpEnv && failWorkerInner->callback->status_ == napi_ok) {
                napi_create_uint32(failWorkerInner->callback->env_, failWorkerInner->error, &jsError);
                args[0] = { jsError };
            } else {
                goto EXIT_CODE;
            }
            if (failWorkerInner->callback->env_ == tmpEnv && failWorkerInner->callback->callback_ != nullptr &&
                failWorkerInner->callback->status_ == napi_ok) {
            napi_get_reference_value(failWorkerInner->callback->env_, failWorkerInner->callback->callback_, &callback);
            napi_get_global(failWorkerInner->callback->env_, &global);
            callStatus = napi_call_function(failWorkerInner->callback->env_, global, callback, 1, args, &result);
            } else {
                goto EXIT_CODE;
            }
            if (callStatus != napi_ok) {
                UPLOAD_HILOGD(UPLOAD_MODULE_JS_NAPI,
                    "Fail callback failed callStatus:%{public}d callback:%{public}p", callStatus, callback);
            }
EXIT_CODE :
            delete failWorkerInner;
            failWorkerInner = nullptr;
            delete work;
            work = nullptr;
        });
    CheckQueueWorkRet(ret, failWorker, work);
}
} // end of OHOS::Request::Upload