for %%v in (shaders/source/*) do ("shaders/glslangValidator.exe" -V shaders/source/%%v -o %%v.spv)
del %1 /q
mkdir %1
for %%v in (*.spv) do move %%v %1 >nul