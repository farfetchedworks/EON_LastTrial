#include "mcv_platform.h"
#include "particles/particle_emitter.h"

class CParticlesResourceType : public CResourceType {
public:
    const char* getExtension(int idx) const { return ".particles"; }
    const char* getName() const { return "Particles"; }
    IResource* create(const std::string& name) const
    {
        json jData = loadJson(name);
        if (jData.empty())
        {
            return nullptr;
        }

        particles::CEmitter* emitter = new particles::CEmitter();
        const bool ok = emitter->parse(jData);
        assert(ok);
        if (!ok)
        {
            delete emitter;
            return nullptr;
        }

        return emitter;
    }
};

template<>
CResourceType* getClassResourceType<particles::CEmitter>() {
    static CParticlesResourceType factory;
    return &factory;
}
// -----------------------------------------


namespace particles
{
    bool CEmitter::parse(const json& jFile)
    {
        maxParticles = jFile.value("max_particles", maxParticles);
        numParticles = jFile.value("num_particles", numParticles);
        interval = jFile.value("interval", interval);

        const std::string& typeStr = jFile["type"];
        
        if(typeStr == "circle")
        {
            type = TEmitterType::Circle;
            typeSize = jFile.value("type_radius", 1.0f);
        }
        else if (typeStr == "circle_perimeter")
        {
            type = TEmitterType::CirclePerimeter;
            typeSize = jFile.value("type_radius", 1.0f);
        }
        else if(typeStr == "sphere")
        {
            type = TEmitterType::Sphere;
            typeSize = jFile.value("type_radius", 1.0f);
        }
        else
        {
            type = TEmitterType::Box;
            typeSize = loadVEC3(jFile, "type_size");
        }

        velocity = loadVEC3(jFile, "velocity");
        explode = jFile.value("explode", explode);
        gravityFactor = jFile.value("gravity_factor", gravityFactor);
        followOwner = jFile.value("follow_owner", followOwner);
        
        lifetime = jFile.value("lifetime", lifetime);
        lifetimeVariation = jFile.value("lifetime_variation", lifetimeVariation);
        additive = jFile.value("additive", additive);

        texture = Resources.get(jFile["texture"])->as<CTexture>();
        for (auto& jEntry : jFile["color"])
        {
            color.set(jEntry[0], loadVEC3(jEntry[1]));
        }
        color.sort();
        for (auto& jEntry : jFile["opacity"])
        {
            opacity.set(jEntry[0], jEntry[1]);
        }
        opacity.sort();
        for (auto& jEntry : jFile["size"])
        {
            size.set(jEntry[0], jEntry[1]);
        }
        size.sort();

        frameSize = loadVEC2(jFile, "frame_size", frameSize);
        frameInitial = jFile.value("frame_initial", frameInitial);
        frameFinal = jFile.value("frame_final", frameFinal);
        frameRate = jFile.value("frame_rate", frameRate);

        return true;
    }

    TParticle CEmitter::generate(MAT44 ownerTransform) const
    {
        TParticle p;

        p.position = generatePosition();
        p.lifetime = 0.f;
        p.maxLifetime = lifetime + Random::range(-lifetimeVariation, lifetimeVariation);
        p.size = size.get(0.f);
        p.opacity = opacity.get(0.f);
        p.color = color.get(0.f);
        p.velocity = velocity;
        p.frameIdx = frameInitial;

        VEC3 origin = VEC3::Zero;
        if (!followOwner)
        {
            p.position = VEC3::Transform(p.position, ownerTransform);
            origin = VEC3::Transform(origin, ownerTransform);
        }

        if (explode)
        {
            const float speed = p.velocity.Length();
            if (speed > 0.f)
            {
                p.velocity = p.position - origin;
                p.velocity.Normalize();
                p.velocity *= speed;
            }
        }

        return p;
    }

    VEC3 CEmitter::generatePosition() const
    {
        float _PI = (float)M_PI;

        switch (type)
        {
            case TEmitterType::Box:
            {
                const VEC3 size = std::get<VEC3>(typeSize);
                return VEC3(Random::range(-size.x, size.x), Random::range(-size.y, size.y), Random::range(-size.z, size.z));
            }
            case TEmitterType::Circle:
            {
                const float radius = std::get<float>(typeSize);
                return yawToVector(Random::range(-_PI, _PI)) * Random::range(0.f, radius);
            }
            case TEmitterType::CirclePerimeter:
            {
                const float radius = std::get<float>(typeSize);
                return yawToVector(Random::range(-_PI, _PI)) * radius;
            }
            case TEmitterType::Sphere:
            {
                const float radius = std::get<float>(typeSize);
                return yawPitchToVector(Random::range(-_PI, _PI), Random::range(-_PI, _PI)) * radius;
            }
            default:;
        }

        return VEC3::Zero;
    }

    bool CEmitter::renderInMenu() const
    {
        return false;
    }
}