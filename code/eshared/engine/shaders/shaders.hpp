/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright � 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef SHADERS_HPP
#define SHADERS_HPP

// make sure this all goes into .data section!

//#ifdef eRELEASE
// include shaders
#include "globals.hpp"

// engine shaders
#include "ps_particles.hpp"
#include "vs_particles.hpp"
#include "vs_instanced_geo.hpp"
#include "ps_forward_light.hpp"
#include "ps_nolight.hpp"
#include "vs_nolight.hpp"
#include "ps_deferred_geo.hpp"
#include "ps_deferred_ambient.hpp"
#include "ps_deferred_light.hpp"
#include "ps_distance.hpp"
#include "vs_distance.hpp"
#include "vs_shadow.hpp"
#include "ps_shadow.hpp"
#include "ps_quad.hpp"
#include "vs_quad.hpp"

// post FX shaders
#include "ps_fx_adjust.hpp"
#include "ps_fx_blur.hpp"
#include "ps_fx_fog.hpp"
#include "ps_fx_distort.hpp"
#include "ps_fx_dof.hpp"
#include "ps_fx_ssao.hpp"
#include "ps_fx_fxaa.hpp"
#include "ps_fx_merge.hpp"
#include "ps_fx_radialblur.hpp"
#include "ps_fx_colorgrading.hpp"
#include "ps_fx_terrain.hpp"
#include "ps_fx_planet.hpp"
#include "ps_fx_ripple.hpp"
#include "ps_fx_microscope.hpp"
#include "ps_fx_interference.hpp"
#include "ps_fx_space.hpp"
#include "ps_fx_motionblur.hpp"
#include "ps_fx_halo.hpp"
#include "ps_fx_caustics.hpp"
#include "ps_fx_debug.hpp"
#include "ps_fx_raytrace.hpp"

// bitmap operator shaders
#include "cs_bmp_adjust.hpp"
#include "cs_bmp_alpha.hpp"
#include "cs_bmp_blur.hpp"
#include "cs_bmp_bump.hpp"
#include "cs_bmp_cells.hpp"
#include "cs_bmp_circle.hpp"
#include "cs_bmp_color.hpp"
#include "cs_bmp_distort.hpp"
#include "cs_bmp_fill.hpp"
#include "cs_bmp_glow.hpp"
#include "cs_bmp_line.hpp"
#include "cs_bmp_mask.hpp"
#include "cs_bmp_merge.hpp"
#include "cs_bmp_multiply.hpp"
#include "cs_bmp_normals.hpp"
#include "cs_bmp_perlin.hpp"
#include "cs_bmp_pixels.hpp"
#include "cs_bmp_rect.hpp"
#include "cs_bmp_rotzoom.hpp"
#include "cs_bmp_sineplasma.hpp"
#include "cs_bmp_twirl.hpp"
#include "cs_bmp_pixelize.hpp"
//#endif

#endif // SHADERS_HPP
