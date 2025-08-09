#include "stdafx.h"
#include "GameObject.h"

using namespace ingame;

// Constructor
GameObject::GameObject()
    : position(0, 0), rotation(0.0f), scale(1.0f, 1.0f)
{
    objects.parent = this;
}

// Updating states of child objects
void GameObject::update( float dt )
{
    begin_update();
    update_children(dt);
    update_sounds(dt);
    end_update();
}

// Begin updating
void GameObject::begin_update()
{
    ingameTRY(
    {
        Direct3D d3d;
        
        // Rotation matrix
        D3DXMATRIX rotation_matrix;
        float angle = rotation * 3.1415926f / 180.0f;
        D3DXMatrixRotationZ(&rotation_matrix, angle);
        
        // Scaling matrix
        D3DXMATRIX scaling_matrix;
        D3DXMatrixScaling(&scaling_matrix, scale.x, scale.y, 1.0f);
        
        // Translation matrix
        D3DXMATRIX translation_matrix;
        D3DXMatrixTranslation(&translation_matrix, position.x, position.y, 0.0f);
        
        // Multiplying matrices
        D3DXMATRIX intermediate_matrix;
        D3DXMATRIX result_matrix;
        D3DXMatrixMultiply(&intermediate_matrix, &rotation_matrix, &translation_matrix);
        D3DXMatrixMultiply(&result_matrix, &scaling_matrix, &intermediate_matrix);
        
        // Pushing transformation to stack
        d3d->push_transform(result_matrix);
        transformation = d3d->get_transform();
        D3DXMatrixInverse(&transformation_inv, NULL, &transformation);
    });
}

// Update children
void GameObject::update_children( float dt )
{
    ingameTRY(
    {
        tanita2_dict objects_copy(objects);
        for (tanita2_dict::iterator i = objects_copy.begin(); objects_copy.end() != i; ++i)
            ((GameObject &)(bp::extract<GameObject &>(i.value()))).update(dt);
    });
}

// End updating
void GameObject::end_update()
{
    ingameTRY(
    {
        Direct3D d3d;
        d3d->pop_transform();
    });
}

// Update sounds
void GameObject::update_sounds( float dt )
{
    tanita2_dict sounds_copy(sounds);
    for (tanita2_dict::iterator i = sounds_copy.begin(); sounds_copy.end() != i; ++i)
        ((SoundID &)(bp::extract<SoundID &>(i.value()))).render(dt);
}

// Loading sound
void GameObject::add_sound( bp::str & name, bp::object & sound_path )
{
    SoundManager sm;
    sounds.setitem(name, bp::object(sm->load(PATH((char *)bp::extract<char *>(sound_path)))));
}

// Get position in absolute coordinates
D3DXVECTOR2 GameObject::get_absolute_position() const
{
    D3DXVECTOR4 p_multiplied;
    D3DXVec2Transform(&p_multiplied, &D3DXVECTOR2(0, 0), &transformation);
    return D3DXVECTOR2(p_multiplied.x, p_multiplied.y);
}

// Get absolute rotation angle
float GameObject::get_absolute_rotation() const
{
    D3DXVECTOR4 A, B;
    D3DXVec2Transform(&A, &D3DXVECTOR2(0, 0), &transformation);
    D3DXVec2Transform(&B, &D3DXVECTOR2(1, 0), &transformation);
    const float result = atan2f(B.y - A.y, B.x - A.x) * 180.0f / 3.1415926f;
    return result < 0 ? result + 360.0f : result;
}

// Convert world coordinates to local
D3DXVECTOR2 GameObject::to_local_coordinates( const D3DXVECTOR2 & v )
{
    // Multiplying v by inverse transformation matrix
    Direct3D d3d;
    D3DXVECTOR4 p_multiplied;
    D3DXVec2Transform(&p_multiplied, &v, &transformation_inv);
    return D3DXVECTOR2(p_multiplied.x, p_multiplied.y);
}

// Cleanup
LayerImage::~LayerImage()
{
    SequenceManager sm;
    sm->del(sequence);
}

// Load image
void LayerImage::load_image( bp::object & texture_path, bool compressed )
{
    SequenceManager sm;
    ingameTRY(sequence = sm->add(StaticSequence(PATH(bp::extract<char *>(texture_path)), compressed)));
}

// Load image from direct path
void LayerImage::direct_load_image( bp::object & texture_path, bool compressed )
{
    SequenceManager sm;
    ingameTRY(sequence = sm->add(StaticSequence(PATH_EXT(bp::extract<char *>(texture_path)), compressed)));
}

// Rendering image
void LayerImage::update( float dt )
{
    begin_update();
    sequence.render(dt);
    update_children(dt);
    end_update();
}
