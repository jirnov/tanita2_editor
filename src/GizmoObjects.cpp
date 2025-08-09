#include "stdafx.h"
#include "GameObject.h"

using namespace ingame;

// Constructor
Gizmo::Gizmo()
{
    SequenceManager sm;
    ingameTRY(sequence = sm->add(GizmoSequence(0, 0, 0)));
    gs = dynamic_cast<GizmoSequence *>(sequence.s);
}

// Rendering
void Gizmo::update( float dt )
{
    begin_update();
    sequence.render(dt);
    end_update();
}

// Constructor
TextObject::TextObject()
{
    SequenceManager sm;
    ingameTRY(sequence = sm->add(TextSequence(0)));
    ts = dynamic_cast<TextSequence *>(sequence.s);
}

// Rendering
void TextObject::update( float dt )
{
    begin_update();
    sequence.render(dt);
    end_update();
}
