/*****************************************************************************
 * Static data
 *****************************************************************************/
 static char module_desc[] = "ring buffer unit test driver";
 /*
 typedef struct
{
	uint16_t	num_of_arg;
	uint8_t 	argtype[UTMGR_MAX_ARGS];
} utmgr_test_argtype_t;
*/
static utmgr_test_argtype_t tc_1_args_desc = {4, {UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_STRING}};
static utmgr_test_argtype_t tc_1_ret_desc = {4, {UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_LWORD}};

static utmgr_test_argtype_t tc_2_args_desc = {4, {UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_LWORD}};
static utmgr_test_argtype_t tc_2_ret_desc = {4, {UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_LWORD, UTMGR_ARG_TYPE_STRING, UTMGR_ARG_TYPE_STRING}}; 
 /*
 typedef struct
{
	char *p_test_id;
	uint8_t (*p_func_tc_prepare)(void *p_in[], void *p_out[]);
	void (*p_func_tc_run)(void);
	void (*p_func_tc_cleanup)(void);
    utmgr_test_argtype_t *p_in_desc;
	utmgr_test_argtype_t *p_out_desc;
} utmgr_test_desc_t;
*/

/*****************************************************************************
 * Static functions
 *****************************************************************************/
static uint8_t tc_1_prepare(void)
{
	SEGGER_RTT_printf(0, "tc 1 prepare\n");
	return (0);
}

static void tc_1_run(void *p_in[], void *p_out[])
{     
   SEGGER_RTT_printf(0, "tc 1 run\n");
   SEGGER_RTT_printf(0, "arg 1: %d 2: %d 3: %s 4: %s\n",
                     *(uint32_t *)p_in[0],
					 *(uint32_t *)p_in[1],
					 (char *)p_in[2],
					 (char *)p_in[3]); 

    strcpy((char *)p_out[0], "test return string 1");
    *(uint32_t *)p_out[1] = 100;
	strcpy((char *)p_out[2], "hello hello");
	*(uint32_t *)p_out[3] = 200;
}

static void tc_1_cleanup(void)
{
    SEGGER_RTT_printf(0, "tc 1 cleanup\n");	
}

static utmgr_test_desc_t tc_1_desc = {"rbf001",
                                      tc_1_prepare,
									  tc_1_run,
									  tc_1_cleanup,
									  &tc_1_args_desc,
									  &tc_1_ret_desc};

static uint8_t tc_2_prepare(void)
{
	SEGGER_RTT_printf(0, "tc 2 prepare\n");
	return (0);
}

static void tc_2_run(void *p_in[], void *p_out[])
{
    SEGGER_RTT_printf(0, "tc 2 run\n");
    SEGGER_RTT_printf(0, "arg 1: %s 2: %s 3: %d 4: %d\n",
					 (char *)p_in[0],
					 (char *)p_in[1],
                     *(uint32_t *)p_in[2],
					 *(uint32_t *)p_in[3]);

    *(uint32_t *)p_out[0] = 100;
	*(uint32_t *)p_out[1] = 200;
    strcpy((char *)p_out[2], "test return string 1");
	strcpy((char *)p_out[3], "hello hello");
}

static void tc_2_cleanup(void)
{
    SEGGER_RTT_printf(0, "tc 2 cleanup\n");	
}

static utmgr_test_desc_t tc_2_desc = {"rbf002",
                                      tc_2_prepare,
									  tc_2_run,
									  tc_2_cleanup,
									  &tc_2_args_desc,
									  &tc_2_ret_desc};									  
static utmgr_test_desc_t *tc_array[] = {&tc_1_desc, &tc_2_desc};

static utmgr_test_desc_t *tc_find(char *p_tc_id)
{
	for (uint8_t i = 0; i < sizeof(tc_array)/sizeof(utmgr_test_desc_t *); i++)
		if (!strcmp(tc_array[i]->p_test_id, p_tc_id))
			return (tc_array[i]);
		
    return (0);
}

static utmgr_module_desc_t module_tc_desc = {module_desc, 2, tc_find};
/*****************************************************************************
 * Interface functions
 *****************************************************************************/

 utmgr_module_desc_t *rbf_get_module_desc(void)
 {
	 return (&module_tc_desc);
 }