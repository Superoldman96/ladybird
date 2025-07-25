/*
 * Copyright (c) 2025, Jelle Raaijmakers <jelle@ladybird.org>
 * Copyright (c) 2025, Lucien Fiorini <lucienfiorini@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/SVGFilterElementPrototype.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/SVG/SVGFEBlendElement.h>
#include <LibWeb/SVG/SVGFEFloodElement.h>
#include <LibWeb/SVG/SVGFEGaussianBlurElement.h>
#include <LibWeb/SVG/SVGFilterElement.h>

namespace Web::SVG {

GC_DEFINE_ALLOCATOR(SVGFilterElement);

SVGFilterElement::SVGFilterElement(DOM::Document& document, DOM::QualifiedName qualified_name)
    : SVGElement(document, qualified_name)
{
}

void SVGFilterElement::initialize(JS::Realm& realm)
{
    WEB_SET_PROTOTYPE_FOR_INTERFACE(SVGFilterElement);
    Base::initialize(realm);
}

void SVGFilterElement::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    SVGURIReferenceMixin::visit_edges(visitor);
}

void SVGFilterElement::apply_presentational_hints(GC::Ref<CSS::CascadedProperties> cascaded_properties) const
{
    Base::apply_presentational_hints(cascaded_properties);
    auto parsing_context = CSS::Parser::ParsingParams { document(), CSS::Parser::ParsingMode::SVGPresentationAttribute };

    auto x_attribute = attribute(AttributeNames::x);
    if (auto x_value = parse_css_value(parsing_context, x_attribute.value_or(String {}), CSS::PropertyID::X))
        cascaded_properties->set_property_from_presentational_hint(CSS::PropertyID::X, x_value.release_nonnull());

    auto y_attribute = attribute(AttributeNames::y);
    if (auto y_value = parse_css_value(parsing_context, y_attribute.value_or(String {}), CSS::PropertyID::Y))
        cascaded_properties->set_property_from_presentational_hint(CSS::PropertyID::Y, y_value.release_nonnull());

    auto width_attribute = attribute(AttributeNames::width);
    if (auto width_value = parse_css_value(parsing_context, width_attribute.value_or(String {}), CSS::PropertyID::Width))
        cascaded_properties->set_property_from_presentational_hint(CSS::PropertyID::Width, width_value.release_nonnull());

    auto height_attribute = attribute(AttributeNames::height);
    if (auto height_value = parse_css_value(parsing_context, height_attribute.value_or(String {}), CSS::PropertyID::Height))
        cascaded_properties->set_property_from_presentational_hint(CSS::PropertyID::Height, height_value.release_nonnull());
}

bool SVGFilterElement::is_presentational_hint(FlyString const& name) const
{
    if (Base::is_presentational_hint(name))
        return true;

    return name.is_one_of(AttributeNames::x, AttributeNames::y, AttributeNames::width, AttributeNames::height);
}

void SVGFilterElement::attribute_changed(FlyString const& name, Optional<String> const& old_value, Optional<String> const& value, Optional<FlyString> const& namespace_)
{
    Base::attribute_changed(name, old_value, value, namespace_);

    if (name == AttributeNames::filterUnits)
        m_filter_units = AttributeParser::parse_units(value.value_or({}));
    else if (name == AttributeNames::primitiveUnits)
        m_primitive_units = AttributeParser::parse_units(value.value_or({}));
}

Optional<Gfx::Filter> SVGFilterElement::gfx_filter()
{
    HashMap<String, Gfx::Filter> result_map;
    Optional<Gfx::Filter> root_filter;

    // https://www.w3.org/TR/filter-effects-1/#element-attrdef-filter-primitive-in
    auto resolve_input_filter = [&](String const& name) -> Optional<Gfx::Filter> {
        // TODO: Add missing ones.
        if (name == "SourceGraphic"sv)
            return {};
        if (name == "SourceAlpha"sv) {
            float matrix[20] = {
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 0, 0,
                0, 0, 0, 1, 0
            };
            return Gfx::Filter::color_matrix(matrix);
        }

        auto filter_from_map = result_map.get(name).copy();

        if (filter_from_map.has_value())
            return filter_from_map;

        return root_filter;
    };

    for_each_child([&](auto& node) {
        if (is<SVGFEFloodElement>(node)) {
            auto& flood_primitive = static_cast<SVGFEFloodElement&>(node);

            root_filter = Gfx::Filter::flood(flood_primitive.flood_color(), flood_primitive.flood_opacity());

            auto result = flood_primitive.result()->base_val();
            if (!result.is_empty()) {
                result_map.set(result, *root_filter);
            }
        } else if (is<SVGFEBlendElement>(node)) {
            auto& blend_primitive = static_cast<SVGFEBlendElement&>(node);

            auto foreground = resolve_input_filter(blend_primitive.in1()->base_val());

            auto background = resolve_input_filter(blend_primitive.in2()->base_val());

            // FIXME: Actually resolve the blend mode
            auto blend_mode = Gfx::CompositingAndBlendingOperator::Normal;

            root_filter = Gfx::Filter::blend(background, foreground, blend_mode);

            auto result = blend_primitive.result()->base_val();
            if (!result.is_empty()) {
                result_map.set(result, *root_filter);
            }
        } else if (is<SVGFEGaussianBlurElement>(node)) {
            auto& blur_primitive = static_cast<SVGFEGaussianBlurElement&>(node);

            auto input = resolve_input_filter(blur_primitive.in1()->base_val());

            auto radius_x = blur_primitive.std_deviation_x()->base_val();
            auto radius_y = blur_primitive.std_deviation_y()->base_val();

            root_filter = Gfx::Filter::blur(radius_x, radius_y, input);

            auto result = blur_primitive.result()->base_val();
            if (!result.is_empty()) {
                result_map.set(result, *root_filter);
            }
        }

        return IterationDecision::Continue;
    });

    return root_filter;
}

// https://drafts.fxtf.org/filter-effects/#element-attrdef-filter-filterunits
GC::Ref<SVGAnimatedEnumeration> SVGFilterElement::filter_units() const
{
    return SVGAnimatedEnumeration::create(realm(), to_underlying(m_filter_units.value_or(SVGUnits::ObjectBoundingBox)));
}

// https://drafts.fxtf.org/filter-effects/#element-attrdef-filter-primitiveunits
GC::Ref<SVGAnimatedEnumeration> SVGFilterElement::primitive_units() const
{
    return SVGAnimatedEnumeration::create(realm(), to_underlying(m_primitive_units.value_or(SVGUnits::UserSpaceOnUse)));
}

// https://drafts.fxtf.org/filter-effects/#element-attrdef-filter-x
GC::Ref<SVGAnimatedLength> SVGFilterElement::x() const
{
    return svg_animated_length_for_property(CSS::PropertyID::X);
}

// https://drafts.fxtf.org/filter-effects/#element-attrdef-filter-y
GC::Ref<SVGAnimatedLength> SVGFilterElement::y() const
{
    return svg_animated_length_for_property(CSS::PropertyID::Y);
}

// https://drafts.fxtf.org/filter-effects/#element-attrdef-filter-width
GC::Ref<SVGAnimatedLength> SVGFilterElement::width() const
{
    return svg_animated_length_for_property(CSS::PropertyID::Width);
}

// https://drafts.fxtf.org/filter-effects/#element-attrdef-filter-height
GC::Ref<SVGAnimatedLength> SVGFilterElement::height() const
{
    return svg_animated_length_for_property(CSS::PropertyID::Height);
}

}
