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

#include <thread>
#include <unistd.h>
#include "log.h"
#include "hisysevent.h"
#include "task_statistics.h"

namespace OHOS::Request::Download {
TaskStatistics &TaskStatistics::GetInstance()
{
    static TaskStatistics instance;
    return instance;
}

void TaskStatistics::ReportTasksSize(uint64_t totalSize)
{
    std::lock_guard<std::mutex> lock(mutex_);
    DayTasksSize_ += totalSize;
}

void TaskStatistics::ReportTasksNumber()
{
    std::lock_guard<std::mutex> lock(mutex_);
    DayTasksNumber_ ++;
}

uint64_t TaskStatistics::GetDayTasksSize() const
{
    return DayTasksSize_;
}

uint32_t TaskStatistics::GetDayTasksNumber() const
{
    return DayTasksNumber_;
}

int32_t TaskStatistics::GetNextReportInterval() const
{
    time_t current = time(nullptr);
        tm localTime = { 0 };
        tm *result = localtime_r(&current, &localTime);
        if (result == nullptr) {
            DOWNLOAD_HILOGE("GetNextReportInterval fail");
            return -1;
        }
    int currentTime = localTime.tm_hour * 3600 + localTime.tm_min * 60 + localTime.tm_sec;
    int sleepTime = 24 * 3600 - currentTime;
    return sleepTime;
}
void TaskStatistics::ReportStatistics() const
{
    OHOS::HiviewDFX::HiSysEvent::Write(OHOS::HiviewDFX::HiSysEvent::Domain::REQUEST,
        REQUEST_SERVICE_START_STATISTIC,
        OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC,
        TASKS_SIZE, DayTasksSize_,
        TASKS_NUMBER, DayTasksNumber_);
}

void TaskStatistics::StartTimerThread()
{
    if (running_) {
        return;
    }

    running_ = true;
    auto fun = [this]() {
        while (true) {
            int32_t nextReportInterval = GetNextReportInterval();
            DOWNLOAD_HILOGD("taskRun next interval: %{public}d", nextReportInterval);
            sleep(nextReportInterval);
            ReportStatistics();
            this->DayTasksNumber_ = 0;
            this->DayTasksSize_ = 0;
        }
    };
    std::thread th = std::thread(fun);
    th.detach();
}
} // namespace OHOS::Request::Download