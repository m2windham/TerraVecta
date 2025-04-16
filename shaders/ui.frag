#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D uiTexture;
uniform vec4 color;
uniform vec4 texCoords; // minU, minV, maxU, maxV
uniform bool useTexture;

void main()
{
    if (useTexture) {
        // Remap the texture coordinates based on texCoords
        vec2 remappedCoords = vec2(
            texCoords.x + TexCoord.x * (texCoords.z - texCoords.x),
            texCoords.y + TexCoord.y * (texCoords.w - texCoords.y)
        );
        
        // Sample texture with remapped coordinates
        vec4 texColor = texture(uiTexture, remappedCoords);
        
        // Apply tint color
        FragColor = texColor * color;
    } else {
        // Just use the color directly
        FragColor = color;
    }
}