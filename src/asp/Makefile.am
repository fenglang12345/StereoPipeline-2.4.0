# __BEGIN_LICENSE__
#  Copyright (c) 2009-2013, United States Government as represented by the
#  Administrator of the National Aeronautics and Space Administration. All
#  rights reserved.
#
#  The NGT platform is licensed under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance with the
#  License. You may obtain a copy of the License at
#  http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
# __END_LICENSE__


#########################################
# sources
#########################################

core_subdirs = Core
core_headers = Core.h

sessions_subdirs = Sessions
sessions_headers = Sessions.h

isisio_subdirs = IsisIO
isisio_headers = IsisIO.h

spiceio_subdirs = SpiceIO
spiceio_headers = SpiceIO.h

tools_subdirs = Tools

mpi_subdirs = MPI

controlnet_subdirs = ControlNetTK

include_HEADERS = $(core_headers) $(spiceio_headers)            \
	  $(isisio_headers) $(sessions_headers) asp_config.h

SUBDIRS = $(core_subdirs) $(spiceio_subdirs) $(isisio_subdirs) \
	  $(sessions_subdirs) $(tools_subdirs) $(mpi_subdirs)  \
	  $(controlnet_subdirs)

CLEANFILES = asp_config.h.pre.in~

#########################################
# general
#########################################

includedir = $(prefix)/include/asp

include $(top_srcdir)/config/rules.mak