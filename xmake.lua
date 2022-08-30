set_project("cyno")
set_version("0.0.1")

add_rules("mode.debug", "mode.release")
add_requires("asio", "http_parser")
add_cxxflags("/EHa", "/EHs")
set_languages("cxx20")
set_warnings("all", "error")

if is_mode("release") then 
    set_optimize("faster")
end

-- libcyno
target("cyno")
    set_kind("static")
    add_files("cyno/**.cpp")
    add_includedirs("cyno")
    add_defines("ASIO_HAS_CO_AWAIT")
    add_packages("asio", "http_parser")

--tests
for _, dir in ipairs(os.files("tests/*.cpp")) do
    target(path.basename(dir), {kind = "binary", files = dir})
        
end