#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;  // Receive texture coordinates from vertex shader

// Output variable for the final fragment color
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D texture1;

void main() {
    // Ambient
    float ambientStrength = 0.3; // Increased ambient strength for better visibility
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Combine lighting with texture
    vec3 texColor = texture(texture1, TexCoord).rgb;
    
    // Fallback if texture sampling fails
    if (length(texColor) < 0.01) {
        texColor = objectColor; // Use object color as fallback
    }
    
    vec3 result = (ambient + diffuse + specular) * texColor;
    FragColor = vec4(result, 1.0);
    
    // Debug visualization - uncomment to see plain colors
    // FragColor = vec4(TexCoord.x, TexCoord.y, 0.0, 1.0); // Shows texture coordinates as colors
}
