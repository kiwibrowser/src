python
import os
import sys

sys.path.insert (0, os.getcwd() + '/..')
import stlport.printers
stlport.printers.register_stlport_printers (None)

# stlport.printers.stlport_version           = 5.2
# stlport.printers.print_vector_with_indices = False

end
