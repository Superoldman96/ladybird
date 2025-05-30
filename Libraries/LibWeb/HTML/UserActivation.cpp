/*
 * Copyright (c) 2024, Jamie Mansfield <jmansfield@cadixdev.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/Bindings/UserActivationPrototype.h>
#include <LibWeb/HTML/UserActivation.h>
#include <LibWeb/HTML/Window.h>

namespace Web::HTML {

GC_DEFINE_ALLOCATOR(UserActivation);

WebIDL::ExceptionOr<GC::Ref<UserActivation>> UserActivation::construct_impl(JS::Realm& realm)
{
    return realm.create<UserActivation>(realm);
}

UserActivation::UserActivation(JS::Realm& realm)
    : PlatformObject(realm)
{
}

void UserActivation::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(UserActivation);
    Base::initialize(realm);
}

// https://html.spec.whatwg.org/multipage/interaction.html#dom-useractivation-hasbeenactive
bool UserActivation::has_been_active() const
{
    // The hasBeenActive getter steps are to return true if this's relevant global object has sticky activation, and false otherwise.
    return as<HTML::Window>(relevant_global_object(*this)).has_sticky_activation();
}

// https://html.spec.whatwg.org/multipage/interaction.html#dom-useractivation-isactive
bool UserActivation::is_active() const
{
    // The isActive getter steps are to return true if this's relevant global object has transient activation, and false otherwise.
    return as<HTML::Window>(relevant_global_object(*this)).has_transient_activation();
}

}
