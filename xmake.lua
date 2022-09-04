set_project("cyno")
set_version("0.0.1")

add_rules("mode.debug", "mode.release")
add_requires("asio", "http_parser", "spdlog", "fmt")
add_cxxflags("/EHa", "/EHs")
set_languages("cxx20")
set_warnings("all")

if is_mode("release") then 
    set_optimize("faster")
end

-- libcyno
target("cyno")
    set_kind("static")
    add_rules("c++.unity_build", {batchsize = 0})
    add_files("src/cyno/**.cpp", {unity_group = "cyno"})
    add_includedirs("src", {public = true})
    add_defines("ASIO_HAS_CO_AWAIT")
    add_packages("asio", "http_parser", "spdlog")

--examples
for _, dir in ipairs(os.files("examples/*.cpp")) do
    target(path.basename(dir))
        set_kind("binary")
        add_files(dir)
        add_deps("cyno")
        add_defines("ASIO_HAS_CO_AWAIT")
        add_packages("asio", "http_parser", "spdlog")
end

--tests
for _, dir in ipairs(os.files("tests/*.cpp")) do
    target(path.basename(dir))
        set_kind("binary")
        add_files(dir)
        add_deps("cyno")
        add_defines("ASIO_HAS_CO_AWAIT")
        add_packages("asio", "http_parser", "spdlog")
        set_group("tests")
end
