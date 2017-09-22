#define MM_SET_EXPR(Expr) _mm_set_ps(Expr, Expr, Expr, Expr)

INTERNAL_FUNCTION void 
SpawnFontain(
    particle_cache* Cache,
    vec3 InitPoint)
{
    TIMED_FUNCTION();

    if(Cache){
        particle_system* System = &Cache->FontainSystem;
        random_series* Entropy = &Cache->ParticleEntropy;

        vec3_4x InitPointWide = Vec3ToWide(InitPoint);

        uint32 ParticleIndex4 = System->NextParticle4++;
        if(System->NextParticle4 >= MAX_PARTICLE_COUNT_4){
            System->NextParticle4 = 0;
        }

        particle_4x* A = System->Particles + ParticleIndex4;

        A->P.x = MM_SET_EXPR(RandomBetween(Entropy, -0.01f, 0.01f));
        A->P.y = MM_SET_EXPR(0.0f);
        A->P.z = MM_SET_EXPR(0.0f);

        A->P += InitPointWide;

        A->dP.x = MM_SET_EXPR(RandomBetween(Entropy, -0.1f, 0.1f));
        A->dP.y = MM_SET_EXPR(RandomBetween(Entropy, 1.0f, 1.4f));
        A->dP.z = MM_SET_EXPR(0.0f);

        A->ddP.x = MM_SET_EXPR(0.0f);
        A->ddP.y = MM_SET_EXPR(-1.0f);
        A->ddP.z = MM_SET_EXPR(0.0f);

        A->C.r = MM_SET_EXPR(1.0f);
        A->C.g = MM_SET_EXPR(1.0f);
        A->C.b = MM_SET_EXPR(1.0f);
        A->C.a = MM_SET_EXPR(4.0f);

        A->dC.r = MM_SET_EXPR(0.0f);
        A->dC.g = MM_SET_EXPR(0.0f);
        A->dC.b = MM_SET_EXPR(0.0f);
        A->dC.a = MM_SET_EXPR(-0.5f);
    }
}

INTERNAL_FUNCTION void 
UpdateAndRenderFontain(
    particle_system* System,
    float DeltaTime,
    vec3 FrameDisplacement, 
    render_group* RenderGroup)
{
    TIMED_FUNCTION();

    //object_transform Transform = DefaultUprightTransform();
    vec3_4x FrameDisplacementWide = Vec3ToWide(FrameDisplacement);

    for(uint32 ParticleIndex4 = 0;
        ParticleIndex4 < MAX_PARTICLE_COUNT_4;
        ParticleIndex4++)
    {
        particle_4x* A = System->Particles + ParticleIndex4;

        A->P += 0.5f * DeltaTime * DeltaTime * A->ddP + DeltaTime * A->dP;
        A->P += FrameDisplacementWide;
        A->dP += DeltaTime * A->ddP;
        A->C += DeltaTime * A->dC;
    
        __m128 mmAlpha = A->C.a;
        mmAlpha = _mm_max_ps(_mm_min_ps(mmAlpha, _mm_set1_ps(1.0f)), _mm_set1_ps(0.0f));

        __m128i mmAlphaMask = _mm_castps_si128(_mm_cmpge_ps(mmAlpha, _mm_set1_ps(0.9f)));
        __m128 TargetAlpha = _mm_sub_ps(_mm_mul_ps(_mm_set1_ps(9.0f), mmAlpha), _mm_set1_ps(8.1f));
        __m128i AndRes = _mm_and_si128(_mm_castps_si128(TargetAlpha), mmAlphaMask);                
        __m128i AndNotRes = _mm_andnot_si128(mmAlphaMask, _mm_castps_si128(mmAlpha));
        mmAlpha = _mm_castsi128_ps(_mm_or_si128(AndRes, AndNotRes));

        /*
        float BounceCoefficient = 0.4f;
        float FrictionCoefficient = 0.8f;
        if(Particle->P.y > 0.0f){
            Particle->P.y = 0.0f;
            Particle->dP.y = -Particle->dP.y * BounceCoefficient;
            Particle->dP.x = Particle->dP.x * FrictionCoefficient;
        }
        */

#if 1

        __m128 temp = _mm_set1_ps(80.0f);

        __m128 mmBounce = _mm_set1_ps(-0.4f);
        __m128 mmFriction = _mm_set1_ps(0.8f);
        __m128 mmBounceMask = _mm_cmplt_ps(A->P.y, _mm_set1_ps(0.0f));
        __m128 mmOld = _mm_andnot_ps(mmBounceMask, A->P.y);

        A->P.y = _mm_or_ps(
            mmOld,
            _mm_and_ps(mmBounceMask, _mm_set1_ps(0.0f)));
        A->dP.y = _mm_or_ps(
            _mm_andnot_ps(mmBounceMask, A->dP.y),
            _mm_and_ps(mmBounceMask, _mm_mul_ps(A->dP.y, mmBounce)));
        A->dP.x = _mm_or_ps(
            _mm_andnot_ps(mmBounceMask, A->dP.x),
            _mm_and_ps(mmBounceMask, _mm_mul_ps(A->dP.x, mmFriction)));
#endif

        for(uint32 SubIndex = 0;
            SubIndex < 4;
            SubIndex++)
        {
            vec3 P = {
                M(A->P.x, SubIndex),
                M(A->P.y, SubIndex),
                M(A->P.z, SubIndex),
            };

            vec4 C = {
                M(A->C.r, SubIndex),
                M(A->C.g, SubIndex),
                M(A->C.b, SubIndex),
                M(mmAlpha, SubIndex),
            };

            if(C.a > 0.0f){
                PushBitmap(RenderGroup, System->BitmapID, 0.2f, P, C);
            }
        }
    }
}

INTERNAL_FUNCTION void 
UpdateAndRenderParticleSystems(
    particle_cache* Cache, 
    float DeltaTime, 
    render_group* RenderGroup,
    vec3 FrameDisplacement)
{
    TIMED_FUNCTION();

    UpdateAndRenderFontain(
        &Cache->FontainSystem, 
        DeltaTime, 
        FrameDisplacement,
        RenderGroup);
    
}

INTERNAL_FUNCTION void 
InitParticleCache(particle_cache* Cache, game_assets* Assets){
    TIMED_FUNCTION();

    ZeroStruct(*Cache);
    Cache->ParticleEntropy = RandomSeed(1234);

#if 1
    Cache->FontainSystem.BitmapID = GetFirstBitmapFrom(Assets, Asset_Heart);
#else
    Cache->FontainSystem.BitmapID = GetFirstBitmapFrom(Assets, Asset_FontGlyph);
#endif

}