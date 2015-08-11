/******************************************************************************/
/**
@file
@author		Graeme Douglas
@details	For more information, see @ref jobmanager.h.
*/
/******************************************************************************/

#include "jobmanager.h"

sjm_error_t
sjm_dequeue_next_job(
	sjm_t		*jobmanager,
	sensor_job_t	*job,
	char		**name
);

sjm_error_t
sjm_init(
	sjm_t			*jobmanager,
	int			maximum_name_size,
	int			maximum_json_tokens
)
{
	err_t				ion_error;
	ion_dictionary_config_info_t	config;
	
	ms_init();
	ion_init_master_table();
	bpptree_init(&(jobmanager->handler));
	ion_error	= ion_find_by_use_master_table(
				&config,
				SJM_ION_DICT_USE_TYPE,
				ION_MASTER_TABLE_FIND_FIRST
			);
	
	if (err_ok == ion_error)
	{
		ion_error	= dictionary_open(
					&(jobmanager->handler),
					&(jobmanager->dictionary),
					&config
				);
		
		if (err_ok != ion_error)
		{
			return SJM_ERROR_DICT_INITIALIZATION;
		}
	}
	else
	{
		ion_error	= ion_master_table_create_dictionary(
					&(jobmanager->handler),
					&(jobmanager->dictionary),
					key_type_char_array,
					maximum_name_size,
					sizeof(sensor_job_t),
					-1
				);
		
		if (err_ok != ion_error)
		{
			return SJM_ERROR_DICT_INITIALIZATION;
		}
	}
	
	if (err_ok != ion_error)
		return SJM_ERROR_DICT_INITIALIZATION;
	
	jobmanager->maximum_name_size
				= maximum_name_size;
	jobmanager->maximum_json_tokens
				= maximum_json_tokens;
	
	jobmanager->queue.head	= NULL;
	jobmanager->queue.tail	= NULL;
	return SJM_ERROR_OK;
}

sjm_error_t
sjm_delete(
	sjm_t			*jobmanager
)
{
	sjm_error_t		error;
	sensor_job_t		job;
	char			*name;
	
	error			= SJM_ERROR_OK;
	
	/* Free the job queue entirely. */
	while (NULL != jobmanager->queue.head && SJM_ERROR_OK == error)
	{
		error		= sjm_dequeue_next_job(jobmanager, &job, &name);
		free(name);
	}
	dictionary_delete_dictionary(&(jobmanager->dictionary));
	return SJM_ERROR_OK;
}

sjm_error_t
sjm_add_job(
	sjm_t			*jobmanager,
	char			*jobname,
	sensor_job_t		*job
)
{
	err_t			ion_error;
	int i;
	char			buffer[jobmanager->maximum_name_size];
	for (i = 0; i < jobmanager->maximum_name_size; i++)
	{
		buffer[i] = '\0';
	}
	strcpy(buffer, jobname);
	ion_error		= dictionary_insert(
					&(jobmanager->dictionary),
					(ion_key_t)buffer,
					(ion_value_t)job
				);
	
	if (err_ok != ion_error)
		return SJM_ERROR_ADD_JOB;
	else
		return SJM_ERROR_OK;
}

sjm_error_t
sjm_perform_job(
	sjm_t			*jobmanager,
	char			*name,
	void			**params,
	void			*retval
)
{
	err_t			ion_error;
	sensor_job_t		job;
	int i;
	char			buffer[jobmanager->maximum_name_size];
	for (i = 0; i < jobmanager->maximum_name_size; i++)
	{
		buffer[i] = '\0';
	}
	strcpy(buffer, name);
	
	ion_error		= dictionary_get(
					&(jobmanager->dictionary),
					(ion_key_t)buffer,
					(ion_value_t)&job
				);
	if (err_ok != ion_error)
	{
		return SJM_ERROR_DICT_GET_FAILURE;
	}
	job.func(params, retval);
	
	return SJM_ERROR_OK;
}

void
sjm_debug_job(
	sjm_t			*jobmanager,
	char			*name
)
{
	err_t			ion_error;
	sensor_job_t		job;
	int i;
	char			buffer[jobmanager->maximum_name_size];
	for (i = 0; i < jobmanager->maximum_name_size; i++)
	{
		buffer[i] = '\0';
	}
	strcpy(buffer, name);
	
	ion_error		= dictionary_get(
					&(jobmanager->dictionary),
					(ion_key_t)buffer,
					(ion_value_t)&job
				);
	
printf("ion_error=%d\n", (int)ion_error);fflush(stdout);
printf("jobname=%s\n", name);fflush(stdout);
printf("job.func=%p\n", job.func);fflush(stdout);
printf("job.needs_execution=%p\n", job.needs_execution);fflush(stdout);
printf("job.last_execution_time=%llu\n", job.last_execution_time);fflush(stdout);
printf("job.last_scheduled_time=%llu\n", job.last_scheduled_time);fflush(stdout);
}

#ifdef  SJM_JSON_HANDLING
sjm_error_t
sjm_request_job(
	sjm_t			*jobmanager,
	char			*json,
	void			*returnval
)
{
	char			*jobname;
	jsmn_parser		jsonparser;
	jsmntok_t		tokens[jobmanager->maximum_json_tokens];
	jsmnerr_t		jsmnerror;
	int			i;
	int			length;
	int			numints;
	char			original;
	sjm_error_t		error;
	
	jsmntok_clear(tokens, jobmanager->maximum_json_tokens);
	jsmn_init(&jsonparser);
	jsmnerror		= jsmn_parse(
					&jsonparser,
					json,
					strlen(json),
					tokens,
					jobmanager->maximum_json_tokens
				);
	if (jsmnerror < 2)
		return SJM_ERROR_UNSUPPORTED_JSON_FORMAT;
	
	/* Must be JSON array. */
	/* First parameter in array must be string identifying query. */
	if (JSMN_ARRAY != tokens[0].type || JSMN_STRING != tokens[1].type)
		return SJM_ERROR_UNSUPPORTED_JSON_FORMAT;
	jobname	= json+tokens[1].start;
	
	length			= 0;
	numints			= 0;
	while (JSMN_PRIMITIVE <= tokens[length].type &&
	       tokens[length].type <= JSMN_STRING &&
	       length <= jobmanager->maximum_json_tokens)
	{
		switch (tokens[length].type)
		{
		 case JSMN_ARRAY:
		 case JSMN_UNINITIALIZED:
			break;
		 case JSMN_STRING:
		 case JSMN_PRIMITIVE:
		 {
			numints++;
		 } break;
		 default:
			return SJM_ERROR_UNSUPPORTED_JSON_FORMAT;
		}
		length++;
	}
	
	if (length < 2)	return SJM_ERROR_UNSUPPORTED_JSON_FORMAT;
	void			*params[length-2];
	/* Since C requires arrays have non-zero size, add 1. :( */
	int			integers[numints+1];
	
	i			= 2;
	numints			= 0;
	while (JSMN_PRIMITIVE <= tokens[i].type &&
	       tokens[i].type <= JSMN_STRING &&
	       i <= jobmanager->maximum_json_tokens)
	{
		switch (tokens[i].type) {
		 case JSMN_ARRAY:
		 case JSMN_UNINITIALIZED:
			break;
		 case JSMN_STRING:
		 {
			params[i-2]	= json+tokens[i].start;
		 } break;
		 case JSMN_PRIMITIVE:
		 {
			/* If this primative is 'true'. */
			if (tokens[i].end - tokens[i].start == 4 &&
			    0==strncmp("true", json+tokens[i].start, 4))
			{
				integers[numints]
						= 1;
				params[i-2]	= integers+numints;
				numints++;
			}
			/* If this primative is 'true'. */
			else if (tokens[i].end - tokens[i].start == 5 &&
			         0==strncmp("false", json+tokens[i].start, 5))
			{
				integers[numints]
						= 0;
				params[i-2]	= integers+numints;
				numints++;
			}
			else
			{
				original	= json[tokens[i].end];
				json[tokens[i].end]
						= '\0';	
				integers[numints]
						= atoi(json+tokens[i].start);
				params[i-2]	= integers+numints;
				json[tokens[i].end]
						= original;
				numints++;
			}
		 } break;
		 default:
			return SJM_ERROR_UNSUPPORTED_JSON_FORMAT;
		}
		i++;
	}
	
	original		= jobname[tokens[1].end];
	json[tokens[1].end]	= '\0';
	
	error			= sjm_perform_job(
					jobmanager,
					jobname,
					params,
					returnval
				);
	
	json[tokens[1].end]	= original;
	
	return error;
}
#endif

/**
@brief		Add a job to the back of the execution queue.
@param		jobmanager
			The jobmanager whose queue we wish to add the job
			to.
@param		job
			The job to add to the back of the queue.
@param		name
			The name for the job. It will also get allocated into
			permanent memory.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_enqueue_job(
	sjm_t		*jobmanager,
	sensor_job_t	*job,
	char		*name
)
{
	if (NULL == jobmanager->queue.tail)
	{
		jobmanager->queue.tail	= malloc(sizeof(sjm_queue_node_t));
		jobmanager->queue.tail->job
					= *job;
		jobmanager->queue.tail->next
					= NULL;
		jobmanager->queue.tail->prev
					= NULL;
		jobmanager->queue.tail->name
					= malloc(strlen(name)+1);
		strcpy(jobmanager->queue.tail->name, name);
		jobmanager->queue.head	= jobmanager->queue.tail;
		return SJM_ERROR_OK;
	}
	
	jobmanager->queue.tail->next	= malloc(sizeof(sjm_queue_node_t));
	if (NULL == jobmanager->queue.tail->next)
	{
		return SJM_ERROR_MEMORY_ALLOCATION_FAILURE;
	}
	jobmanager->queue.tail->next->prev
					= jobmanager->queue.tail;
	jobmanager->queue.tail->next->next
					= NULL;
	jobmanager->queue.tail->next->job
					= *job;
	jobmanager->queue.tail->next->name
					= malloc(strlen(name)+1);
	strcpy(jobmanager->queue.tail->next->name, name);
	jobmanager->queue.tail		= jobmanager->queue.tail->next;
	
	return SJM_ERROR_OK;
}

/**
@brief		Get the next job to execute from front of queue.
@param		jobmanager
			The jobmanager whose queue we wish to retrieve from.
@param		job
			A pointer to a job variable that can safely be written
			to (it is already allocated).
@param		name
			A pointer to a character array pointer that should
			be set to the job name. NOTE THAT THE CALLEE
			IS RESPONSIBLE FOR FREEING THE NAME ONCE IT IS DONE
			WITH IT.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_dequeue_next_job(
	sjm_t		*jobmanager,
	sensor_job_t	*job,
	char		**name
)
{
	sjm_queue_node_t	*tempjobp;
	
	if (NULL == jobmanager->queue.head)
	{
		return SJM_ERROR_NO_MORE_QUEUED_JOBS;
	}
	
	*name			= jobmanager->queue.head->name;
	*job			= jobmanager->queue.head->job;
	tempjobp		= jobmanager->queue.head->next;
	if (NULL != tempjobp)
	{
		tempjobp->prev	= NULL;
	}
	free(jobmanager->queue.head);
	jobmanager->queue.head	= tempjobp;
	if (NULL == jobmanager->queue.head)
	{
		jobmanager->queue.tail
				= NULL;
	}
	
	return SJM_ERROR_OK;
}

/**
@brief		Update a job in the key-value store.
@param		jobmanager
			The job manager to update the job in.
@param		job
			The updated job to store.
@param		name
			The name (key) of the job to update.
@returns	@c SJM_ERROR_OK on success, otherwise an error code
		appropriately describing the situation. 
*/
sjm_error_t
sjm_update_job(
	sjm_t		*jobmanager,
	sensor_job_t	*job,
	char		*name
)
{
	err_t		ion_error;
	char		buffer[jobmanager->maximum_name_size];
	int		i;
	
	for (i = 0; i < jobmanager->maximum_name_size; i++)
	{
		buffer[i] = '\0';
	}
	strcpy(buffer, name);
	
	ion_error	= dictionary_update(
				&(jobmanager->dictionary),
				(ion_key_t)buffer,
				(ion_value_t)(job)
			);
	
	if (err_ok != ion_error)
	{
		return SJM_ERROR_DICT_UPDATE_FAILURE;
	}
	
	return SJM_ERROR_OK;
}

sjm_error_t
sjm_execute_queued_job(
	sjm_t		*jobmanager
)
{
	sjm_error_t	error;
	sensor_job_t	job;
	char		*name;
	
	error			= sjm_dequeue_next_job(jobmanager, &job, &name);
	if (SJM_ERROR_NO_MORE_QUEUED_JOBS == error)
	{
		return SJM_ERROR_OK;
	}
	else if (SJM_ERROR_OK != error)
	{
		return error;
	}
	
	job.func(NULL, NULL);
	
	job.last_execution_time	= ms_milliseconds();
	error			= sjm_update_job(jobmanager, &job, name);
	free(name);
	
	return error;
}

sjm_error_t
sjm_queue_scheduled_jobs(
	sjm_t		*jobmanager
)
{
	dict_cursor_t	*cursor;
	predicate_t	predicate;
	ion_record_t	record;
	err_t		error;
	sjm_error_t	sjmerror;
	sensor_job_t	*job;
	milliseconds_t	milliseconds;
	char		keydata[jobmanager->dictionary.instance->record.key_size];
	char		valuedata[jobmanager->dictionary.instance->record.value_size];
	record.key			= (void *)keydata;
	record.value			= (void *)valuedata;
	sjmerror			= SJM_ERROR_OK;
	if (NULL == record.key || NULL == record.value)
	{
		return SJM_ERROR_MEMORY_ALLOCATION_FAILURE;
	}
	
	/* Get a cursor over all the items in the job dictionary. */
	cursor				= NULL;
	error				= dictionary_build_predicate(
						&predicate,
						predicate_all_records
					);
	error				= dictionary_find(
						&(jobmanager->dictionary),
						&predicate,
						&cursor
					);
	while (cs_end_of_results != (error = cursor->next(cursor, &record)))
	{
		/* Scheduling logic. */
		milliseconds		= ms_milliseconds();
		
		job			= (void *)(record.value);
		if (job->needs_execution(job, MS_GET_BASE_MILLIS, milliseconds))
		{
			sjmerror	= sjm_enqueue_job(
						jobmanager,
						job,
						(char *)(record.key)
					);
			if (SJM_ERROR_OK != sjmerror)
			{
				cursor->destroy(&cursor);
				return error;
			}
			job->last_scheduled_time
					= ms_milliseconds();
			sjm_update_job(jobmanager, job, (char *)(record.key));
		}
	}
	cursor->destroy(&cursor);
	
	return SJM_ERROR_OK;
}
