def if_cuda(if_true, if_false = []): return if_false
def if_cuda_is_configured(if_true, if_false = []): return if_false
def cuda_default_copts(): return []
def cuda_library(name, **kwargs): native.cc_library(name = name, **kwargs)
