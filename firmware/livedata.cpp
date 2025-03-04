#include "wideband_config.h"
#include "livedata.h"

#include "lambda_conversion.h"
#include "sampling.h"
#include "pump_dac.h"
#include "heater_control.h"
#include "max31855.h"
#include "fault.h"

#include <rusefi/arrays.h>
#include <rusefi/fragments.h>

static livedata_common_s livedata_common;
static livedata_afr_s livedata_afr[AFR_CHANNELS];

void SamplingUpdateLiveData()
{
    for (int ch = 0; ch < AFR_CHANNELS; ch++)
    {
        volatile struct livedata_afr_s *data = &livedata_afr[ch];

        data->lambda = GetLambda(ch);
        data->temperature = GetSensorTemperature(ch);
        data->nernstVoltage = GetNernstDc(ch);
        data->pumpCurrentTarget = GetPumpCurrent(ch);
        data->pumpCurrentMeasured = GetPumpNominalCurrent(ch);
        data->heaterDuty = GetHeaterDuty(ch);
        data->esr = GetSensorInternalResistance(ch);
        data->fault = (uint8_t)GetCurrentFault(ch);
        data->heaterState = (uint8_t)GetHeaterState(ch);
    }

    livedata_common.vbatt = GetInternalBatteryVoltage(0);
}

template<>
const livedata_common_s* getLiveData(size_t)
{
    return &livedata_common;
}

template<>
const struct livedata_afr_s * getLiveData(size_t ch)
{
    if (ch < AFR_CHANNELS)
    {
        return &livedata_afr[ch];
    }

    return nullptr;
}

static const FragmentEntry fragments[] = {
    decl_frag<livedata_common_s>{},
    decl_frag<livedata_afr_s, 0>{},
    decl_frag<livedata_afr_s, 1>{},
    decl_frag<livedata_egt_s, 0>{},
    decl_frag<livedata_egt_s, 1>{},
};

FragmentList getFragments() {
	return { fragments, efi::size(fragments) };
}
