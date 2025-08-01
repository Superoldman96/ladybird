/*
 * Copyright (c) 2018-2023, Andreas Kling <andreas@ladybird.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/DOM/Range.h>
#include <LibWeb/Dump.h>
#include <LibWeb/Layout/TextNode.h>
#include <LibWeb/Layout/Viewport.h>
#include <LibWeb/Painting/PaintableBox.h>
#include <LibWeb/Painting/StackingContext.h>
#include <LibWeb/Painting/ViewportPaintable.h>

namespace Web::Layout {

GC_DEFINE_ALLOCATOR(Viewport);

Viewport::Viewport(DOM::Document& document, GC::Ref<CSS::ComputedProperties> style)
    : BlockContainer(document, &document, move(style))
{
}

Viewport::~Viewport() = default;

GC::Ptr<Painting::Paintable> Viewport::create_paintable() const
{
    return Painting::ViewportPaintable::create(*this);
}

void Viewport::visit_edges(Visitor& visitor)
{
    Base::visit_edges(visitor);
    if (!m_text_blocks.has_value())
        return;

    for (auto& text_block : *m_text_blocks) {
        for (auto& text_position : text_block.positions)
            visitor.visit(text_position.dom_node);
    }
}

Vector<Viewport::TextBlock> const& Viewport::text_blocks()
{
    if (!m_text_blocks.has_value())
        update_text_blocks();

    return *m_text_blocks;
}

void Viewport::update_text_blocks()
{
    StringBuilder builder(StringBuilder::Mode::UTF16);
    size_t current_start_position = 0;
    Vector<TextPosition> text_positions;
    Vector<TextBlock> text_blocks;

    for_each_in_inclusive_subtree([&](auto const& layout_node) {
        if (layout_node.display().is_none() || !layout_node.first_paintable() || !layout_node.first_paintable()->is_visible())
            return TraversalDecision::Continue;

        if (layout_node.is_box() || layout_node.is_generated()) {
            if (!builder.is_empty()) {
                text_blocks.append({ builder.to_utf16_string(), text_positions });
                current_start_position = 0;
                text_positions.clear_with_capacity();
                builder.clear();
            }
            return TraversalDecision::Continue;
        }

        if (auto* text_node = as_if<Layout::TextNode>(layout_node)) {
            // https://html.spec.whatwg.org/multipage/interaction.html#inert-subtrees
            // When a node is inert:
            // - The user agent should ignore the node for the purposes of find-in-page.
            if (auto& dom_node = const_cast<DOM::Text&>(text_node->dom_node()); !dom_node.is_inert()) {
                if (text_positions.is_empty()) {
                    text_positions.empend(dom_node);
                } else {
                    text_positions.empend(dom_node, current_start_position);
                }

                auto const& current_node_text = text_node->text_for_rendering();
                current_start_position += current_node_text.length_in_code_units();
                builder.append(current_node_text);
            }
        }

        return TraversalDecision::Continue;
    });

    if (!builder.is_empty())
        text_blocks.append({ builder.to_utf16_string(), text_positions });

    m_text_blocks = move(text_blocks);
}

}
