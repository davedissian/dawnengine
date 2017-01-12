/*
 * Dawn Engine
 * Written by David Avedissian (c) 2012-2017 (git@dga.me.uk)
 */
#include "Common.h"
#include "io/InputStream.h"
#include "renderer/Material.h"

namespace dw {

Material::Material(Context* context) : Resource(context) {
}

Material::~Material() {
}

bool Material::beginLoad(InputStream& src) {
    getLog().error("Material loading unimplemented");
    return false;
}

void Material::endLoad() {
}
}
