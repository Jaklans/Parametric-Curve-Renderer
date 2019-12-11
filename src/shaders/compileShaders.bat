mkdir assets\shaders
for %%v in (shaders/source/*) do ("shaders/glslangValidator.exe" -V shaders/source/%%v -o assets/shaders/%%v.spv)
mkdir %1
for %%v in (assets/shaders/*.spv) do copy assets\shaders\%%v %1 >nul