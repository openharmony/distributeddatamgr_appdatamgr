/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "preferences_value.h"

namespace OHOS {
namespace NativePreferences {
PreferencesValue::PreferencesValue(const PreferencesValue &preferencesValue)
{
    if (this == &preferencesValue) {
        return;
    }
    value_ = preferencesValue.value_;
}

PreferencesValue::PreferencesValue(PreferencesValue &&preferencesValue) noexcept
{
    if (this == &preferencesValue) {
        return;
    }
    value_ = std::move(preferencesValue.value_);
}

PreferencesValue::PreferencesValue(int value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(int64_t value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(float value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(double value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(bool value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(const char *value)
{
    PreferencesValue(std::string(value));
}

PreferencesValue::PreferencesValue(std::string value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(std::vector<double> value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(std::vector<std::string> value)
{
    value_ = value;
}

PreferencesValue::PreferencesValue(std::vector<bool> value)
{
    value_ = value;
}

PreferencesValue &PreferencesValue::operator=(PreferencesValue &&preferencesValue) noexcept
{
    if (this == &preferencesValue) {
        return *this;
    }
    value_ = std::move(preferencesValue.value_);
    return *this;
}

PreferencesValue &PreferencesValue::operator=(const PreferencesValue &preferencesValue)
{
    if (this == &preferencesValue) {
        return *this;
    }
    value_ = preferencesValue.value_;
    return *this;
}

bool PreferencesValue::IsInt() const
{
    return std::holds_alternative<int>(value_);
}

bool PreferencesValue::IsLong() const
{
    return std::holds_alternative<int64_t>(value_);
}

bool PreferencesValue::IsFloat() const
{
    return std::holds_alternative<float>(value_);
}

bool PreferencesValue::IsDouble() const
{
    return std::holds_alternative<double>(value_);
}

bool PreferencesValue::IsBool() const
{
    return std::holds_alternative<bool>(value_);
}

bool PreferencesValue::IsString() const
{
    return std::holds_alternative<std::string>(value_);
}

bool PreferencesValue::IsDoubleArray() const
{
    return std::holds_alternative<std::vector<double>>(value_);
}

bool PreferencesValue::IsStringArray() const
{
    return std::holds_alternative<std::vector<std::string>>(value_);
}

bool PreferencesValue::IsBoolArray() const
{
    return std::holds_alternative<std::vector<bool>>(value_);
}

PreferencesValue::operator int() const
{
    return std::get<int>(value_);
}

PreferencesValue::operator int64_t() const
{
    return std::get<int64_t>(value_);
}

PreferencesValue::operator float() const
{
    return std::get<float>(value_);
}

PreferencesValue::operator double() const
{
    return std::get<double>(value_);
}

PreferencesValue::operator bool() const
{
    return std::get<bool>(value_);
}

PreferencesValue::operator std::string() const
{
    return std::get<std::string>(value_);
}

PreferencesValue::operator std::vector<double>() const
{
    return std::get<std::vector<double>>(value_);
}

PreferencesValue::operator std::vector<bool>() const
{
    return std::get<std::vector<bool>>(value_);
}

PreferencesValue::operator std::vector<std::string>() const
{
    return std::get<std::vector<std::string>>(value_);
}

bool PreferencesValue::operator==(const PreferencesValue &value)
{
    return (this->value_ == value.value_);
}
} // End of namespace NativePreferences
} // End of namespace OHOS
