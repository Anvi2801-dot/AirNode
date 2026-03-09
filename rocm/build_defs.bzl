def if_rocm(if_true, if_false = []): return if_false
def if_rocm_is_configured(if_true, if_false = []): return if_false
def rocm_default_copts(): return []
def rocm_library(name, **kwargs): native.cc_library(name = name, **kwargs)
def rocm_copts(): return []
