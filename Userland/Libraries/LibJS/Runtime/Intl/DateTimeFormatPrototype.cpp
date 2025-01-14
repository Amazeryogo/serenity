/*
 * Copyright (c) 2021, Tim Flynn <trflynn89@pm.me>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibJS/Runtime/GlobalObject.h>
#include <LibJS/Runtime/Intl/DateTimeFormatPrototype.h>
#include <LibUnicode/DateTimeFormat.h>

namespace JS::Intl {

// 11.4 Properties of the Intl.DateTimeFormat Prototype Object, https://tc39.es/ecma402/#sec-properties-of-intl-datetimeformat-prototype-object
DateTimeFormatPrototype::DateTimeFormatPrototype(GlobalObject& global_object)
    : PrototypeObject(*global_object.object_prototype())
{
}

void DateTimeFormatPrototype::initialize(GlobalObject& global_object)
{
    Object::initialize(global_object);

    auto& vm = this->vm();

    // 11.4.2 Intl.DateTimeFormat.prototype [ @@toStringTag ], https://tc39.es/ecma402/#sec-intl.datetimeformat.prototype-@@tostringtag
    define_direct_property(*vm.well_known_symbol_to_string_tag(), js_string(vm, "Intl.DateTimeFormat"), Attribute::Configurable);

    u8 attr = Attribute::Writable | Attribute::Configurable;
    define_native_function(vm.names.resolvedOptions, resolved_options, 0, attr);
}

// 11.4.7 Intl.DateTimeFormat.prototype.resolvedOptions ( ), https://tc39.es/ecma402/#sec-intl.datetimeformat.prototype.resolvedoptions
JS_DEFINE_NATIVE_FUNCTION(DateTimeFormatPrototype::resolved_options)
{
    // 1. Let dtf be the this value.
    // 2. If the implementation supports the normative optional constructor mode of 4.3 Note 1, then
    //     a. Set dtf to ? UnwrapDateTimeFormat(dtf).
    // 3. Perform ? RequireInternalSlot(dtf, [[InitializedDateTimeFormat]]).
    auto* date_time_format = TRY(typed_this_object(global_object));

    // 4. Let options be ! OrdinaryObjectCreate(%Object.prototype%).
    auto* options = Object::create(global_object, global_object.object_prototype());

    // 5. For each row of Table 7, except the header row, in table order, do
    //     a. Let p be the Property value of the current row.
    //     b. If p is "hour12", then
    //         i. Let hc be dtf.[[HourCycle]].
    //         ii. If hc is "h11" or "h12", let v be true.
    //         iii. Else if, hc is "h23" or "h24", let v be false.
    //         iv. Else, let v be undefined.
    //     c. Else,
    //         i. Let v be the value of dtf's internal slot whose name is the Internal Slot value of the current row.
    //     d. If the Internal Slot value of the current row is an Internal Slot value in Table 4, then
    //         i. If dtf.[[DateStyle]] is not undefined or dtf.[[TimeStyle]] is not undefined, then
    //             1. Let v be undefined.
    //     e. If v is not undefined, then
    //         i. Perform ! CreateDataPropertyOrThrow(options, p, v).
    MUST(options->create_data_property_or_throw(vm.names.locale, js_string(vm, date_time_format->locale())));
    MUST(options->create_data_property_or_throw(vm.names.calendar, js_string(vm, date_time_format->calendar())));
    MUST(options->create_data_property_or_throw(vm.names.numberingSystem, js_string(vm, date_time_format->numbering_system())));
    MUST(options->create_data_property_or_throw(vm.names.timeZone, js_string(vm, date_time_format->time_zone())));

    if (date_time_format->has_hour_cycle()) {
        MUST(options->create_data_property_or_throw(vm.names.hourCycle, js_string(vm, date_time_format->hour_cycle_string())));

        switch (date_time_format->hour_cycle()) {
        case Unicode::HourCycle::H11:
        case Unicode::HourCycle::H12:
            MUST(options->create_data_property_or_throw(vm.names.hour12, Value(true)));
            break;
        case Unicode::HourCycle::H23:
        case Unicode::HourCycle::H24:
            MUST(options->create_data_property_or_throw(vm.names.hour12, Value(false)));
            break;
        }
    }

    if (!date_time_format->has_date_style() && !date_time_format->has_time_style()) {
        MUST(for_each_calendar_field(global_object, *date_time_format, [&](auto& option, auto const& property, auto const&) -> ThrowCompletionOr<void> {
            using ValueType = typename RemoveReference<decltype(option)>::ValueType;

            if (!option.has_value())
                return {};

            if constexpr (IsIntegral<ValueType>) {
                TRY(options->create_data_property_or_throw(property, Value(*option)));
            } else {
                auto name = Unicode::calendar_pattern_style_to_string(*option);
                TRY(options->create_data_property_or_throw(property, js_string(vm, name)));
            }

            return {};
        }));
    }

    if (date_time_format->has_date_style())
        MUST(options->create_data_property_or_throw(vm.names.dateStyle, js_string(vm, date_time_format->date_style_string())));
    if (date_time_format->has_time_style())
        MUST(options->create_data_property_or_throw(vm.names.timeStyle, js_string(vm, date_time_format->time_style_string())));

    // 6. Return options.
    return options;
}

}
