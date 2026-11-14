// Fails the compile for arm-linux-gnueabihf when the escape would take the NEON form,
// or when a Boost.PFR of the build machine is visible to the cross compiler.
#ifdef __ARM_NEON
#error "__ARM_NEON is defined: the armhf job must build the scalar form of the escape"
#endif
#if __has_include(<boost/pfr/core_name.hpp>)
#error "boost/pfr/core_name.hpp is visible: the armhf job must not build typed.hpp over Boost.PFR"
#endif
