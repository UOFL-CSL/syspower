/*
 * Stand-in for syspower's NVIDIA GPU model (src/mdl/nvml/nvml.c).
 * The real one needs NVIDIA's nvml.h, which normally only comes with CUDA.
 * Returning an error means "this model can't measure", so GPU groups
 * (only created for known NVIDIA parts) report a failed measurement.
 */
#include <src/dconf.h> // syspower's headers need this first
#include <src/grp/grp.h>
#include <src/mdl/nvml/nvml.h>

int mdl_nvml_gpu(struct grp_group *grp)
{
	(void)grp;
	return -1;
}
