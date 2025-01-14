#  Copyright (C) 2012  Equinor ASA, Norway.
#
#  The file 'obs_vector.py' is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.
from cwrap import BaseCClass
from res import ResPrototype, _lib
from res.enkf.config import EnkfConfigNode
from res.enkf.enums import EnkfObservationImplementationType
from res.enkf.observations.block_observation import BlockObservation
from res.enkf.observations.gen_observation import GenObservation
from res.enkf.observations.summary_observation import SummaryObservation


class ObsVector(BaseCClass):
    TYPE_NAME = "obs_vector"

    _alloc = ResPrototype(
        "void* obs_vector_alloc(enkf_obs_impl_type, char*, enkf_config_node, int)",
        bind=False,
    )
    _free = ResPrototype("void  obs_vector_free( obs_vector )")
    _get_state_kw = ResPrototype("char* obs_vector_get_state_kw( obs_vector )")
    _get_key = ResPrototype("char* obs_vector_get_key( obs_vector )")
    _iget_node = ResPrototype("void* obs_vector_iget_node( obs_vector, int)")
    _get_num_active = ResPrototype("int   obs_vector_get_num_active( obs_vector )")
    _iget_active = ResPrototype("bool  obs_vector_iget_active( obs_vector, int)")
    _get_impl_type = ResPrototype(
        "enkf_obs_impl_type obs_vector_get_impl_type( obs_vector)"
    )
    _install_node = ResPrototype(
        "void  obs_vector_install_node(obs_vector, int, void*)"
    )
    _get_next_active_step = ResPrototype(
        "int   obs_vector_get_next_active_step(obs_vector, int)"
    )
    _has_data = ResPrototype(
        "bool  obs_vector_has_data(obs_vector , bool_vector , enkf_fs)"
    )
    _get_config_node = ResPrototype(
        "enkf_config_node_ref obs_vector_get_config_node(obs_vector)"
    )
    _get_total_chi2 = ResPrototype(
        "double obs_vector_total_chi2(obs_vector, enkf_fs, int)"
    )
    _get_obs_key = ResPrototype("char*  obs_vector_get_obs_key(obs_vector)")

    def __init__(self, observation_type, observation_key, config_node, num_reports):
        """
        @type observation_type: EnkfObservationImplementationType
        @type observation_key: str
        @type config_node: EnkfConfigNode
        @type num_reports: int
        """
        assert isinstance(observation_type, EnkfObservationImplementationType)
        assert isinstance(observation_key, str)
        assert isinstance(config_node, EnkfConfigNode)
        assert isinstance(num_reports, int)
        c_ptr = self._alloc(observation_type, observation_key, config_node, num_reports)
        super().__init__(c_ptr)

    def getDataKey(self):
        """@rtype: str"""
        return self._get_state_kw()

    def getObservationKey(self):
        """@rtype: str"""
        return self.getKey()

    def getKey(self):
        return self._get_key()

    def getObsKey(self):
        return self._get_obs_key()

    def getNode(self, index):
        """@rtype: SummaryObservation or BlockObservation or GenObservation"""

        pointer = self._iget_node(index)

        node_type = self.getImplementationType()
        if node_type == EnkfObservationImplementationType.SUMMARY_OBS:
            return SummaryObservation.createCReference(pointer, self)
        elif node_type == EnkfObservationImplementationType.BLOCK_OBS:
            return BlockObservation.createCReference(pointer, self)
        elif node_type == EnkfObservationImplementationType.GEN_OBS:
            return GenObservation.createCReference(pointer, self)
        else:
            raise AssertionError(f"Node type '{node_type}' currently not supported!")

    def __iter__(self):
        """Iterate over active report steps; return node"""
        for step in self.getStepList():
            yield self.getNode(step)

    def getStepList(self):
        """
        Will return an IntVector with the active report steps.
        """
        return _lib.obs_vector_get_step_list(self)

    def activeStep(self):
        """Assuming the observation is only active for one report step, this
        method will return that report step - if it is active for more
        than one report step the method will raise an exception.
        """
        step_list = self.getStepList()
        if len(step_list) == 1:
            return step_list[0]
        else:
            raise ValueError(
                "The activeStep() method can *ONLY* be called "
                "for obervations with one active step"
            )

    def firstActiveStep(self):
        """@rtype: int"""
        step_list = self.getStepList()
        if len(step_list) > 0:
            return step_list[0]
        else:
            raise ValueError(
                "the firstActiveStep() method cannot be called with no active steps."
            )

    def getActiveCount(self):
        """@rtype: int"""
        return len(self)

    def __len__(self):
        return self._get_num_active()

    def isActive(self, index):
        """@rtype: bool"""
        return self._iget_active(index)

    def getNextActiveStep(self, previous_step=-1):
        """@rtype: int"""
        return self._get_next_active_step(previous_step)

    def getImplementationType(self):
        """@rtype: EnkfObservationImplementationType"""
        return self._get_impl_type()

    def installNode(self, index, node):
        assert isinstance(node, SummaryObservation)
        node.convertToCReference(self)
        self._install_node(index, node.from_param(node))

    def getConfigNode(self):
        """@rtype: EnkfConfigNode"""
        return self._get_config_node().setParent(self)

    def hasData(self, active_mask, fs):
        """@rtype: bool"""
        return self._has_data(active_mask, fs)

    def free(self):
        self._free()

    def __repr__(self):
        return (
            f"ObsVector(data_key = {self.getDataKey()}, "
            f"key = {self.getKey()}, obs_key = {self.getObsKey()}, "
            f"num_active = {len(self)}) {self._ad_str()}"
        )

    def getTotalChi2(self, fs, realization_number):
        """@rtype: float"""
        return self._get_total_chi2(fs, realization_number)
