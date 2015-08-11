/**
@author		Graeme Douglas
@brief
@details
@copyright	Copyright 2015 Graeme Douglas
@license	Licensed under the Apache License, Version 2.0 (the "License");
		you may not use this file except in compliance with the License.
		You may obtain a copy of the License at
			http://www.apache.org/licenses/LICENSE-2.0

@par
		Unless required by applicable law or agreed to in writing,
		software distributed under the License is distributed on an
		"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
		either express or implied. See the License for the specific
		language governing permissions and limitations under the
		License.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "../CuTest.h"
#include "../../src/jobmanager.h"

/* These are the test jobs. */
void testjob_1(void **params, void *returned)
{
	int x;
	int y;
	x			= *((int *)params[0]);
	y			= *((int *)params[1]);
	
	*((int*)returned)	= x + y;
}

void testjob_2(void **params, void *returned)
{
	int x;
	int y;
	int mybool;
	
	x				= *((int *)params[0]);
	y				= *((int *)params[1]);
	mybool				= *((int *)params[2]);
	if (mybool)
	{
		*((int *)returned)	= x+y;
	}
	else
	{
		*((int*)returned)	= -1*(x+y);
	}
}

struct testjob_3_type { int a; int b; };
void testjob_3(void **params, void *returned)
{
	int				x;
	char				*y;
	int				mybool;
	struct testjob_3_type *		returner;
	
	returner			= (struct testjob_3_type *)returned;
	x				= *((int *)params[0]);
	y				= ((char *)params[1]);
	mybool				= *((int *)params[2]);
	if (mybool)
	{
		returner->a		= x+atoi(y);
	}
	else
	{
		returner->a		= -1*(x+atoi(y));
	}
	returner->b			= 97;
}

void testschedulejob_1(void **params, void *returned)
{
	printf("Job 1 executed.\n");fflush(stdout);
}

void testschedulejob_2(void **params, void *returned)
{
	printf("Job 2 executed.\n");fflush(stdout);
}

void testschedulejob_3(void **params, void *returned)
{
	printf("Job 3 executed.\n");fflush(stdout);
}

sjm_bool_t
always_activate(
	sensor_job_t		*job,
	milliseconds_t		epoch,
	milliseconds_t		absolute
)
{
	return true;
}

sjm_bool_t
activate_if_not_executed_or_scheduled_within_last_second(
	sensor_job_t		*job,
	milliseconds_t		epoch,
	milliseconds_t		absolute
)
{
	if (absolute - job->last_execution_time > 1000 &&
	    absolute - job->last_scheduled_time > 1000)
		return true;
	
	return false;
}

void test_jobmanager_nonjson_generic(
	CuTest			*tc,
	int			maximum_name_size,
	int			maximum_json_tokens,
	job_function		func,
	char			*jobname,
	void			**params,
	void			*returnval
)
{
	sjm_t		jobmanager;
	sensor_job_t	job;
	sjm_error_t	error;
	
	error		= sjm_init(
				&jobmanager,
				maximum_name_size,
				maximum_json_tokens
			);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	job.func	= func;
	error		= sjm_add_job(&jobmanager, jobname, &job);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	error		= sjm_perform_job(
				&jobmanager,
				jobname,
				params,
				returnval
			);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	error		= sjm_delete(&jobmanager);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
}

void test_jobmanager_json_generic(
	CuTest			*tc,
	int			maximum_name_size,
	int			maximum_json_tokens,
	job_function		func,
	char			*jobname,
	char			*json,
	void			*returnval
)
{
	sjm_t		jobmanager;
	sensor_job_t	job;
	sjm_error_t	error;
	
	error		= sjm_init(
				&jobmanager,
				maximum_name_size,
				maximum_json_tokens
			);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	job.func	= func;
	error		= sjm_add_job(&jobmanager, jobname, &job);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	error		= sjm_request_job(
				&jobmanager,
				json,
				returnval
			);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	error		= sjm_delete(&jobmanager);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
}

void test_jobmanager_scheduled_generic(
	CuTest			*tc,
	int			maximum_name_size,
	int			queue_loops,
	int			exec_loops,
	int			numjobs,
	sensor_job_t		*jobs,
	char			**names
)
{
	sjm_t		jobmanager;
	sjm_error_t	error;
	int		i;
	error		= sjm_init(
				&jobmanager,
				maximum_name_size,
				5
			);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	/* Add all jobs! */
	for (i = 0; i < numjobs; i++)
	{
		error		= sjm_add_job(&jobmanager, names[i], jobs+i);
		CuAssertTrue(tc, SJM_ERROR_OK == error);
	}
	
	for (i = 0; i < queue_loops || i < exec_loops; i++)
	{
		if (i < queue_loops)
		{
			error	= sjm_queue_scheduled_jobs(&jobmanager);
			CuAssertTrue(tc, SJM_ERROR_OK == error);
		}
		if (i < exec_loops)
		{
			error	= sjm_execute_queued_job(&jobmanager);
			CuAssertTrue(tc, SJM_ERROR_OK == error);
		}
	}
	
	CuAssertTrue(tc, SJM_ERROR_OK == error);
	
	error		= sjm_delete(&jobmanager);
	CuAssertTrue(tc, SJM_ERROR_OK == error);
}

void test_jobmanager_nonjson_1(CuTest *tc)
{
	int		maximum_name_size;
	int		maximum_json_tokens;
	job_function	func;
	char		*jobname;
	void*		params[2];
	int		returnval;
	int		x;
	int		y;
	
	params[0]		= &x;
	params[1]		= &y;
	
	maximum_name_size	= 20;
	maximum_json_tokens	= 12;
	func			= testjob_1;
	jobname			= "TESTJOB1";
	x			= 1;
	y			= 2;
	
	test_jobmanager_nonjson_generic(
		tc,
		maximum_name_size,
		maximum_json_tokens,
		func,
		jobname,
		params,
		&returnval
	);
	
	CuAssertTrue(tc, x+y == returnval);
}

void test_jobmanager_json_1(CuTest *tc)
{
	int		maximum_name_size;
	int		maximum_json_tokens;
	job_function	func;
	char		*jobname;
	char		*json;
	int		returnval;
	
	maximum_name_size	= 20;
	maximum_json_tokens	= 12;
	func			= testjob_1;
	jobname			= "TESTJOB1";
	json			= "[ \"TESTJOB1\", 1, 2 ]";
	char			mutablejson[strlen(json)];
	strcpy(mutablejson, json);
	
	test_jobmanager_json_generic(
		tc,
		maximum_name_size,
		maximum_json_tokens,
		func,
		jobname,
		mutablejson,
		&returnval
	);
	
	CuAssertTrue(tc, 3 == returnval);
}

void test_jobmanager_json_2(CuTest *tc)
{
	int		maximum_name_size;
	int		maximum_json_tokens;
	job_function	func;
	char		*jobname;
	char		*json;
	int		returnval;
	
	maximum_name_size	= 20;
	maximum_json_tokens	= 12;
	func			= testjob_2;
	jobname			= "TESTJOB2";
	json			= "[ \"TESTJOB2\", 1, 2, false ]";
	char			mutablejson[strlen(json)];
	strcpy(mutablejson, json);
	
	test_jobmanager_json_generic(
		tc,
		maximum_name_size,
		maximum_json_tokens,
		func,
		jobname,
		mutablejson,
		&returnval
	);
	
	CuAssertTrue(tc, -3 == returnval);
}

void test_jobmanager_json_3(CuTest *tc)
{
	int		maximum_name_size;
	int		maximum_json_tokens;
	job_function	func;
	char		*jobname;
	char		*json;
	int		returnval;
	
	maximum_name_size	= 20;
	maximum_json_tokens	= 12;
	func			= testjob_2;
	jobname			= "TESTJOB2";
	json			= "[ \"TESTJOB2\", -7, 2, true ]";
	char			mutablejson[strlen(json)];
	strcpy(mutablejson, json);
	
	test_jobmanager_json_generic(
		tc,
		maximum_name_size,
		maximum_json_tokens,
		func,
		jobname,
		mutablejson,
		&returnval
	);
	
	CuAssertTrue(tc, -5 == returnval);
}

void test_jobmanager_json_4(CuTest *tc)
{
	int		maximum_name_size;
	int		maximum_json_tokens;
	job_function	func;
	char		*jobname;
	char		*json;
	struct testjob_3_type
			returnval;
	
	maximum_name_size	= 20;
	maximum_json_tokens	= 12;
	func			= testjob_3;
	jobname			= "TESTJOB3";
	json			= "[ \"TESTJOB3\", -7, \"2\", true ]";
	char			mutablejson[strlen(json)];
	strcpy(mutablejson, json);
	
	test_jobmanager_json_generic(
		tc,
		maximum_name_size,
		maximum_json_tokens,
		func,
		jobname,
		mutablejson,
		&returnval
	);
	
	CuAssertTrue(tc, -5 == returnval.a);
	CuAssertTrue(tc, 97 == returnval.b);
}

void test_jobmanager_scheduling_1(CuTest *tc)
{
	int		maximum_name_size	= 10;
	int		queue_loops		= 1;
	int		exec_loops		= 1;
	int		num_jobs		= 1;
	sensor_job_t	jobs[num_jobs];
	char		*names[num_jobs];
	jobs[0].func				= testschedulejob_1;
	jobs[0].needs_execution			= always_activate;
	names[0]	= "job1";
	
	test_jobmanager_scheduled_generic(
		tc,
		maximum_name_size,
		queue_loops,
		exec_loops,
		num_jobs,
		jobs,
		names
	);
	
	test_jobmanager_scheduled_generic(
		tc,
		maximum_name_size,
		queue_loops,
		exec_loops,
		num_jobs,
		jobs,
		names
	);
}

void test_jobmanager_scheduling_2(CuTest *tc)
{
	int		maximum_name_size	= 10;
	int		queue_loops		= 2;
	int		exec_loops		= 4;
	int		num_jobs		= 2;
	sensor_job_t	jobs[num_jobs];
	char		*names[num_jobs];
	jobs[0].func				= testschedulejob_1;
	jobs[0].needs_execution			= always_activate;
	names[0]	= "job1";
	jobs[1].func				= testschedulejob_2;
	jobs[1].needs_execution			= always_activate;
	names[1]	= "job2";
	
	test_jobmanager_scheduled_generic(
		tc,
		maximum_name_size,
		queue_loops,
		exec_loops,
		num_jobs,
		jobs,
		names
	);
	
	test_jobmanager_scheduled_generic(
		tc,
		maximum_name_size,
		queue_loops,
		exec_loops,
		num_jobs,
		jobs,
		names
	);
}

void test_jobmanager_scheduling_3(CuTest *tc)
{
	int		maximum_name_size	= 10;
	int		queue_loops		= 2;
	int		exec_loops		= 4;
	int		num_jobs		= 2;
	sensor_job_t	jobs[num_jobs];
	char		*names[num_jobs];
	jobs[0].func				= testschedulejob_1;
	jobs[0].needs_execution			= always_activate;
	jobs[0].last_execution_time		= ms_milliseconds();
	names[0]	= "job1";
	jobs[1].func				= testschedulejob_2;
	jobs[1].needs_execution			=
				activate_if_not_executed_or_scheduled_within_last_second;
	jobs[1].last_execution_time		= ms_milliseconds();
	names[1]	= "job2";
	
	printf("Sleeping... zzzzz\n");fflush(stdout);
	sleep(3);
	
	test_jobmanager_scheduled_generic(
		tc,
		maximum_name_size,
		queue_loops,
		exec_loops,
		num_jobs,
		jobs,
		names
	);
	
	/* Update these since copies are stored in dictionary. */
	jobs[0].last_execution_time		= ms_milliseconds();
	jobs[1].last_execution_time		= ms_milliseconds();
	
	test_jobmanager_scheduled_generic(
		tc,
		maximum_name_size,
		queue_loops,
		exec_loops,
		num_jobs,
		jobs,
		names
	);
}

CuSuite *JobManagerGetSuite()
{
	CuSuite *suite = CuSuiteNew();
	
	SUITE_ADD_TEST(suite, test_jobmanager_nonjson_1);
	SUITE_ADD_TEST(suite, test_jobmanager_json_1);
	SUITE_ADD_TEST(suite, test_jobmanager_json_2);
	SUITE_ADD_TEST(suite, test_jobmanager_json_3);
	SUITE_ADD_TEST(suite, test_jobmanager_json_4);
	SUITE_ADD_TEST(suite, test_jobmanager_scheduling_1);
	SUITE_ADD_TEST(suite, test_jobmanager_scheduling_2);
	SUITE_ADD_TEST(suite, test_jobmanager_scheduling_3);
	
	return suite;
}

void runAllTests_jobmanager()
{
	CuString *output = CuStringNew();
	CuSuite* suite = JobManagerGetSuite();
	
	//CuSuiteAddSuite(suite, JobManagerGetSuite());
	
	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	
	CuSuiteDelete(suite);
	CuStringDelete(output);
}
