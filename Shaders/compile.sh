# TODO: look into https://github.com/google/shaderc
# TODO: move glsl file here to compile?
# TODO: might not be the best way to compile shaders...

glslc="/Users/jacwilso/Documents/vulkansdk-macos-1.1.126.0/macOS/bin/glslc"

for vert in $(find . -type f -name '*.vert'); do
    eval "${glslc}" "${vert}" -o "${vert}.spv"
done

for frag in $(find . -type f -name '*.frag'); do
    eval "${glslc}" "${frag}" -o "${frag}.spv"
done