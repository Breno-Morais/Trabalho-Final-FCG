#version 330 core

// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int object_id;

// Posição da Fonte de Luz
uniform vec4 light_pos;
// Direção da Fonte de Luz
uniform vec4 light_dir;
uniform vec3 MKa;
uniform vec3 MKd;
uniform vec3 MKs;
uniform vec3 MKe;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;
out vec3 cor;

void main()
{
    if(object_id == 5)
        gl_Position = projection * model * model_coefficients;
    else
        gl_Position = projection * view * model * model_coefficients;

    position_world = model * model_coefficients;

    position_model = model_coefficients;

    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    texcoords = texture_coefficients;

    if(object_id == 0)
    {
        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
        vec4 camera_position = inverse(view) * origin;

        vec4 p = position_world;
        vec4 pM = position_model;

        vec4 n = normalize(normal);
        vec4 l = normalize(light_pos - p);
        float angle = dot(-l, light_dir);
        float angFech = 30.0f;

        vec4 v = normalize(camera_position - p);
        vec4 h = normalize(v+l);

        vec3 Ka = MKa;
        vec3 Kd = MKd; // Refletância difusa
        vec3 Ks = MKs; // Refletância especular
        float q = 20.0; // Expoente especular para o modelo de iluminação de Phong

        vec3 I = vec3(1.0,1.0,0.8);
        vec3 Ia = vec3(0.01,0.01,0.01);
        float lambert_diffuse_term = max(0,dot(n,l));
        vec3 ambient_term = Ka*Ia;
        vec3 blinn_phong_specular_term  = Ks*I*pow(max(0,dot(n,h)), q);
        float ml = pow((1 - lambert_diffuse_term),20);

        if(acos(angle) < radians(angFech))
        {
            cor = Kd*I*(lambert_diffuse_term + 0.01) + ambient_term + blinn_phong_specular_term;
        }
        else
            cor = ambient_term + (Kd*I*lambert_diffuse_term*0.01f);
    }
    else
        cor = vec3(0.0f, 0.0f, 0.0f);
}

