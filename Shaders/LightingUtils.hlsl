#define MaxLights 16

struct Light
{
    float3 strength; //光源颜色（三光通用）
    float falloffStart; //点光灯和聚光灯的开始衰减距离
    float3 direction; //方向光和聚光灯的方向向量
    float falloffEnd; //点光和聚光灯的衰减结束距离
    float3 position; //点光和聚光灯的坐标
    float spotPower; //聚光灯因子中的幂参数
};

struct Material
{
    float4 diffuseAlbedo; //材质反照率
    float3 fresnelR0; //RF(0)值，即材质的反射属性
    float roughness; //材质的粗糙度
};

float CalcAttenuation(float d, float falloffEnd, float falloffStart)
{
    //d是离灯光的距离
    float att = saturate((falloffEnd - d) / (falloffEnd - falloffStart));
    return att;
}

float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVector)
{
    //根据schlick function计算出入射光线被反射的百分比
    float3 reflectPercent = R0 + (1.0f - R0) * pow(1.0f - saturate(dot(normal, lightVector)), 5.0f);
    return reflectPercent;

}

float3 BlinnPhong(Material mat, float3 normal, float3 toEye, float3 lightVec, float3 lightStrength)
{
    float m = (1.0f - mat.roughness) * 256.0f; //粗糙度因子里的m值
    float3 halfVec = normalize(lightVec + toEye); //半角向量
    float roughtnessFactor = (m + 8.0f) * pow(max(dot(normal, halfVec), 0.0f), m) / 8.0f; //粗糙度因子
    float3 fresnelFactor = SchlickFresnel(mat.fresnelR0, halfVec, lightVec); //fresnel因子
    
    float3 specAlbedo = fresnelFactor * roughtnessFactor; //镜面反射反照率=fresnel因子*粗糙度因子
    specAlbedo = specAlbedo / (specAlbedo + 1.0f); //将镜面反射反照率缩放到[0，1]
    
    float3 diff = lightStrength * (mat.diffuseAlbedo.rgb + specAlbedo); //漫反射+高光反射=入射光量*总的反照率
    return diff; //返回漫反射+高光反射

}

float3 ComputeDirectionalLight(Light light, Material mat, float3 normal, float3 toEye)
{
    float3 lightVec = -light.direction;
    float3 lightStrenght = max(dot(normal, lightVec), 0.0f) * light.strength;
    
    return BlinnPhong(mat, normal, toEye, lightVec, lightStrenght);
}

float3 ComputePointlLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = light.position - pos; //顶点指向点光源的光向量
    float distance = length(lightVec); //顶点和光源的距离（向量模长）
    
    if (distance > light.falloffEnd)
    {
        return 0; //如果大于衰减距离直接返回0，省的计算了
    }
    
    lightVec /= distance; //归一化成单位向量
    float nDotl = max(dot(lightVec, normal), 0); //点积不能小于0
    float3 lightStrenght = nDotl * light.strength; //点光再单位面积上的辐照度（没考虑衰减）
    
    //调用衰减函数
    float att = CalcAttenuation(distance, light.falloffEnd, light.falloffStart);
    lightStrenght *= att; //衰减后的单位面积辐照度
    
    //计算点光的漫反射和高光反射
    return BlinnPhong(mat, normal, toEye, lightVec, lightStrenght);
}

float3 ComputeSpotlLight(Light light, Material mat, float3 pos, float3 normal, float3 toEye)
{
    float3 lightVec = light.position - pos; //顶点指向聚光灯光源的光向量
    float distance = length(lightVec); //顶点和光源的距离（向量模长）
    
    if (distance > light.falloffEnd)
    {
        return 0; //如果大于衰减距离直接返回0，省的计算了
    }
    
    lightVec /= distance; //归一化成单位向量
    float nDotl = max(dot(lightVec, normal), 0); //点积不能小于0
    float3 lightStrenght = nDotl * light.strength; //点光再单位面积上的辐照度（没考虑衰减）
    
    //调用衰减函数
    float att = CalcAttenuation(distance, light.falloffEnd, light.falloffStart);
    lightStrenght *= att; //衰减后的单位面积辐照度
    
    //计算聚光灯衰减因子
    float spotFactor = pow(max(dot(-lightVec, light.direction), 0), light.spotPower);
    lightStrenght *= spotFactor;
    
    //计算点光的漫反射和高光反射
    return BlinnPhong(mat, normal, toEye, lightVec, lightStrenght);
}

float4 ComputeLighting(Light lights[MaxLights], Material mat, float3 pos, float3 normal, float3 toEye, float3 shadowFactor)
{
    float3 res = 0.0f;
    int i = 0;
    
#if(NUM_DIR_LIGHTS > 0)
    for (i = 0; i < NUM_DIR_LIGHTS; i++)
    {
        res += shadowFactor[i] * ComputeDirectionalLight(lights[i], mat, normal, toEye);
    }
#endif
    
#if(NUM_POINT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i++)
    {
        res += ComputePointLight(lights[i], mat, pos, normal, toEye);
    }
#endif
    
#if(NUM_SPOT_LIGHTS > 0)
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; i++)
    {
        res += ComputeSpotLight(lights[i], mat, pos, normal, toEye);
    }
#endif
    
    return float4(res, 0.0f);
}