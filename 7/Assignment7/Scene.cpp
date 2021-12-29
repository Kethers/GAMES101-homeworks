//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"

void Scene::buildBVH()
{
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        if (objects[k]->hasEmit())
        {
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum)
            {
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
    const Ray &ray,
    const std::vector<Object *> &objects,
    float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k)
    {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear)
        {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }

    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TODO: Implement Path Tracing Algorithm here

    Vector3f L_dir{}, L_indir{};

    Intersection inter = intersect(ray);
    if (!inter.happened)
    {
        return {};
    }

    // sample ray hit the light source
    Material *&material = inter.m;
    if (material->hasEmission())
    {
        return depth == 0 ? material->getEmission() : Vector3f{};
    }

    Vector3f &p = inter.coords;
    Vector3f normal = normalize(inter.normal);

    std::function<Vector3f(Vector3f)> format = [](Vector3f v)
    {
        v.x = clamp(0.f, 1.f, v.x);
        v.y = clamp(0.f, 1.f, v.y);
        v.z = clamp(0.f, 1.f, v.z);
        return v;
    };

    // direct illumination: hit the object, then sample the light source uniformly
    Intersection lights_src_sample;
    float light_src_pdf = 0.0f;
    sampleLight(lights_src_sample, light_src_pdf);

    Vector3f point_to_light = lights_src_sample.coords - p;
    Vector3f point_to_light_dir = point_to_light.normalized();
    Ray ray_obj2light(p, point_to_light_dir);
    Intersection is_blocked = intersect(ray_obj2light);
    if (point_to_light.norm() - is_blocked.distance < EPSILON)
    {
        // L_dir = L_i * f_r * cos theta * cos theta' / |x'-p|^2 / pdf_light
        L_dir = lights_src_sample.emit * material->eval(ray.direction, point_to_light_dir, normal) * dotProduct(point_to_light_dir, normal) * dotProduct(-point_to_light_dir, lights_src_sample.normal) / (dotProduct(point_to_light, point_to_light) * light_src_pdf);
        L_dir = format(L_dir);
    }

    // indirect illumination, Russian Roulette test
    if (get_random_float() < RussianRoulette)
    {
        Vector3f obj2obj_dir = material->sample(ray.direction, normal).normalized();
        float hemis_pdf = material->pdf(ray.direction, obj2obj_dir, normal);
        // L_indir = shade(q, -wi) * f_r * cos theta / pdf_hemi / P_RR
        L_indir = castRay(Ray(p, obj2obj_dir), depth + 1) * material->eval(ray.direction, obj2obj_dir, normal) * dotProduct(obj2obj_dir, normal) / (hemis_pdf * RussianRoulette);
        L_indir = format(L_indir);
    }

    return depth == 0 ? format(L_dir + L_indir) : L_dir + L_indir;
}