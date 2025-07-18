/*
    ChibiOS - Copyright (C) 2006,2007,2008,2009,2010,2011,2012,2013,2014,
              2015,2016,2017,2018,2019,2020,2021 Giovanni Di Sirio.

    This file is part of ChibiOS.

    ChibiOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 3 of the License.

    ChibiOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * @file    rt/src/chinstances.c
 * @brief   OS instances code.
 *
 * @addtogroup instances
 * @details OS instances management.
 * @{
 */

#include "ch.h"

/*===========================================================================*/
/* Module local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Module exported variables.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Module local types.                                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Module local variables.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Module local functions.                                                   */
/*===========================================================================*/

#if (CH_CFG_NO_IDLE_THREAD == FALSE) || defined(__DOXYGEN__)
/**
 * @brief   This function implements the idle thread infinite loop.
 * @details The function puts the processor in the lowest power mode capable
 *          to serve interrupts.<br>
 *          The priority is internally set to the minimum system value so
 *          that this thread is executed only if there are no other ready
 *          threads in the system.
 *
 * @param[in] p         the thread parameter, unused in this scenario
 */
static void __idle_thread(void *p) {

  (void)p;

  while (true) {
    /*lint -save -e522 [2.2] Apparently no side effects because it contains
      an asm instruction.*/
    port_wait_for_interrupt();
    /*lint -restore*/
    CH_CFG_IDLE_LOOP_HOOK();
  }
}
#endif /* CH_CFG_NO_IDLE_THREAD == FALSE */

/*===========================================================================*/
/* Module exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   Initializes a system instance.
 * @note    The system instance is in I-Lock state after initialization.
 *
 * @param[out] oip      pointer to an @p os_instance_t object
 * @param[in] oicp      pointer to an @p os_instance_config_t object
 *
 * @special
 */
void chInstanceObjectInit(os_instance_t *oip,
                          const os_instance_config_t *oicp) {
  core_id_t core_id;

  /* Registering into the global system structure.*/
#if CH_CFG_SMP_MODE == TRUE
  core_id = port_get_core_id();
#else
  core_id = 0U;
#endif
  chDbgAssert(ch_system.instances[core_id] == NULL, "instance already registered");
  ch_system.instances[core_id] = oip;

  /* Core associated to this instance.*/
  oip->core_id = core_id;

  /* Keeping a reference to the configuration data.*/
  oip->config = oicp;

  /* Port initialization for the current instance.*/
  port_init(oip);

  /* Ready list initialization.*/
  ch_pqueue_init(&oip->rlist.pqueue);

#if (CH_CFG_USE_REGISTRY == TRUE) && (CH_CFG_SMP_MODE == FALSE)
  /* Registry initialization when SMP mode is disabled.*/
  __reg_object_init(&oip->reglist);
#endif

#if CH_CFG_SMP_MODE == FALSE
  /* RFCU initialization when SMP mode is disabled.*/
  __rfcu_object_init(&oip->rfcu);
#endif

  /* Virtual timers list initialization.*/
  __vt_object_init(&oip->vtlist);

  /* Debug support initialization.*/
  __dbg_object_init(&oip->dbg);

#if CH_DBG_TRACE_MASK != CH_DBG_TRACE_MASK_DISABLED
  /* Trace buffer initialization.*/
  __trace_object_init(&oip->trace_buffer);
#endif

  /* Statistics initialization.*/
#if CH_DBG_STATISTICS == TRUE
  __stats_object_init(&oip->kernel_stats);
#endif

  /* Now this instructions flow becomes the main thread or the idle thread
     depending on the CH_CFG_NO_IDLE_THREAD setting.*/
  {
#if CH_CFG_NO_IDLE_THREAD == FALSE
    const THD_DECL(main_thd_desc,
                   "main", oicp->cstack_base, oicp->cstack_end,
                   NORMALPRIO, NULL, NULL, oip
    );

    oip->rlist.current = chThdObjectInit(&oip->mainthread, &main_thd_desc);
#else
    const THD_DECL(idle_thd_desc,
                   "idle", oicp->cstack_base, oicp->cstack_end,
                   IDLEPRIO, NULL, NULL, oip
    );

    oip->rlist.current = chThdObjectInit(&oip->idlethread, &idle_thd_desc);
#endif
  }

#if CH_CFG_USE_REGISTRY == TRUE
  REG_INSERT(oip, oip->rlist.current);
#endif

  /* Setting up the caller as current thread.*/
  oip->rlist.current->state = CH_STATE_CURRENT;

#if CH_DBG_STATISTICS == TRUE
  /* Starting measurement for this thread.*/
  chTMStartMeasurementX(&oip->rlist.current->stats);
#endif

  /* User instance initialization hook.*/
  CH_CFG_OS_INSTANCE_INIT_HOOK(oip);

#if CH_CFG_NO_IDLE_THREAD == FALSE
  {
    const THD_DECL(idle_thd_desc,
                   "idle", oicp->idlestack_base, oicp->idlestack_end,
                   IDLEPRIO, __idle_thread, NULL, oip
    );

#if CH_DBG_FILL_THREADS == TRUE
    __thd_stackfill((uint8_t *)idle_thd_desc.wbase,
                    (uint8_t *)idle_thd_desc.wend);
#endif

    /* This thread has the lowest priority in the system, its role is just to
       serve interrupts in its context while keeping the lowest energy saving
       mode compatible with the system status.*/
    (void) chThdSpawnRunningI(&oip->idlethread, &idle_thd_desc);
  }
#endif /* CH_CFG_NO_IDLE_THREAD == FALSE */
}

/** @} */
