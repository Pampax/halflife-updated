# Tests unitaires de la logique de clamp des cvars, sans dependance au moteur Half-Life.
# Cible independante de "hl" : ne lie ni l'engine, ni le SDK, ni les headers GoldSrc.
add_executable(relicrush_clamp_tests
	relicrush_clamp_tests.cpp
	../relicrush_clamp.cpp
	../relicrush_clamp.h)

target_compile_features(relicrush_clamp_tests PRIVATE cxx_std_17)

add_test(NAME relicrush_clamp_tests COMMAND relicrush_clamp_tests)
