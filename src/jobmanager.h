/******************************************************************************/
/**
@file
@author		Graeme Douglas
@brief		A simple job manager for sensor node applications. Jobs are
		stored in IonDB.
@details	Retrieving data off of sensor networks via any sort of protocol,
		over a network or possibly a serial port, is made easier
		when "requests" or "jobs" can be processed on device.
		This code makes managing jobs much simpler. Once a number
		of jobs have been created, jobs can be processed using
		either direct C calls or JSON arrays, having the format:
		
			[<job-name>, <parameter list>]
*/
/******************************************************************************/

#ifndef JOB_MANAGER_H
#define JOB_MANAGER_H

#ifdef  __cplusplus
extern "C" {
#endif

/**
@brief		The use type for the scheduler dictionary.
*/
#define SJM_ION_DICT_USE_TYPE	1

/**
@brief		Do not define if no JSON handling is required.
*/
#define SJM_JSON_HANDLING

#include "iondb/dictionary.h"
#include "iondb/bpptreehandler.h"
#include "iondb/ion_master_table.h"
#ifdef  SJM_JSON_HANDLING
#include "jsmn/jsmn.h"
#endif
#include "millisec.h"

/* Forward declarations for resolve typing issues. */
typedef struct sensor_job	sensor_job_t;

/**
@brief		A boolean type.
*/
typedef char			sjm_bool_t;

/**
@brief		True value, if not defined.
*/
#ifndef true
#define true ((char)1)
#endif

/**
@brief		False value, if not defined.
*/
#ifndef false
#define false ((char)0)
#endif

/**
@brief		Job function. Every job have this type of signature.
@details	This is a type signature for functions that can be called
		as jobs. Inside such functions parameters must be converted
		to the appropriate types from void*'s.
@param		params
			A pointer to a an array of void pointers,
			expected to be pointers to data the job
			will need.
@param		returned
			A pointer that may be written to determine
			the result of the job.
*/
typedef void (*job_function)(void** params, void* returned);

/**
@brief		Checks if a job needs to be scheduled for execution.
@param		job
			A reference to the job object that is being considered
			for scheduled execution.
@param		epoch
			The first known time for the scheduler. Use this
			to roughly determine when to execute a job. This
			number should be in units of  milliseconds.
@param		elapsed
			The number of milliseconds elapsed since the epoch.
*/
typedef sjm_bool_t (*activation_function)(sensor_job_t* job, milliseconds_t epoch, milliseconds_t elapsed);

/**
@brief		A job is a way of making functions callable at run-time.
		That is, we wish to be able to ask an embedded device
		to execute some function by sending it a message, either
		by the serial (TTY), RF, or a network. This represents
		the lowest level of handling: it simply manages jobs
		and makes them callable. How the message to execute
		a job is provided to the device is another matter.
*/
struct sensor_job
{
	job_function		func;	/**< Function to call during job
					     execution. */
	activation_function	needs_execution;
					/**< Function to call to check if
					     the function job needs executing
					     at present. */
	milliseconds_t		last_execution_time;
					/**< Last time this function was
					     executed. */
	milliseconds_t		last_scheduled_time;
					/**< The last time it was added to
 *					     the execution queue. */
};

/**
@brief		Sensor job queue node.
*/
typedef struct sjm_queue_node
{
	sensor_job_t		job;	/**< Job queued for execution. */
	struct sjm_queue_node	*next;	/**< Next node in queue. */
	struct sjm_queue_node	*prev;	/**< Previous node in queue. */
	char			*name;	/**< Job name. */
} sjm_queue_node_t;

/**
@brief		Sensor job queue.
*/
typedef struct sjm_queue
{
	sjm_queue_node_t	*head;	/**< Head of queue. */
	sjm_queue_node_t	*tail;	/**< Tail of queue. */
} sjm_queue_t;

/**
@brief		The job manager.
@details	This keeps all information need for managing jobs. This
		object needs to be kept in memory so long as the sensor
		is to process jobs.
*/
typedef struct sensor_job_manager
{
	dictionary_handler_t	handler;		/**< IonDB dictionary
							     method handler. */
	dictionary_t		dictionary;		/**< IonDB dictionary
							     control struct. */
	int			maximum_name_size;	/**< Maximum job name
							     size. */
	int			maximum_json_tokens;	/**< Maximum number
							     of json tokens. */
	sjm_queue_t		queue;			/**< Queue of jobs to
							     execute. */
} sjm_t;

/**
@brief		The errors that the sensor job manager may throw.
*/
typedef enum    sensor_job_manager_error
{
	SJM_ERROR_OK,				/**< No error. */
	SJM_ERROR_DICT_INITIALIZATION,		/**< IonDB dictionary could
						     not be initialized. */
	SJM_ERROR_DICT_UPDATE_FAILURE,		/**< Dictionary update failure.
						*/
	SJM_ERROR_DICT_GET_FAILURE,		/**< Dictionary get failure. */
	SJM_ERROR_ADD_JOB,			/**< IonDB dictionary could
						     not add job. */
	SJM_ERROR_GET_JOB,			/**< IonDB dictionary could
						     not get job. */
	SJM_ERROR_UNSUPPORTED_JSON_FORMAT,	/**< Unsupported JSON input. */
	SJM_ERROR_NO_MORE_QUEUED_JOBS,		/**< No queued jobs to execute.
						*/
	SJM_ERROR_MEMORY_ALLOCATION_FAILURE,	/**< Memory could not be
						     could not be allocated. */
} sjm_error_t;


/**
@brief		Initialize a job manager.
@details	This will open the IonDB dictionary needed for the manager.
		It will also setup any other control information necessary.
@param		jobmanager
			A pointer to the job manager structure to initialize.
			Note that this must already be allocated, and will
			not be malloc'd during initialization.
@param		maximum_name_size
			The maximum number of characters any job name
			can take. Note that, in order to work with IonDB,
			names will be copied and padded (IonDB stores
			fixed-length keys without doing any padding for you,
			meaning that it will continue to read junk past the
			end of short key. This code handles that appropriately.)
@param		maximum_json_tokens
			The maximum number of tokens each job request
			can use. Space for exactly this many tokens
			will be created (on the stack).
			
			Note, effectively this is the maximum number
			of elements any parameter array can have -1 since
			the array object itself counts as a token.
			If you want to be able to pass in up to 10
			parameters per job, for instance, specify this
			as 12.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_init(
	sjm_t			*jobmanager,
	int			maximum_name_size,
	int			maximum_json_tokens
);

/**
@brief		Delete/destroy a job manager.
@details	This will complete destroy anything to do with the job
		manager.
@param		jobmanager
			A pointer to the job manager structure to destroy.
			This WILL NOT free the pointer, that is up to the
			callee.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_delete(
	sjm_t			*jobmanager
);

/**
@brief		Add a named job to manage.
@param		jobmanager
			A pointer to the job manager structure that should
			manage the new job.
@param		jobname
			The unique name of the job to perform. This can be
			shorter than the maximum name length, but anything
			longer may result in interesting behaviour.
@param		job
			A pointer to the job that is to be managed. This
			contains a pointer to the job function that
			will be called when the job is performed.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_add_job(
	sjm_t			*jobmanager,
	char			*jobname,
	sensor_job_t		*job
);

/**
@brief		Perform a named job with given parameters and extract
		the return data.
@details	If the job allocates any data on the heap, it must be manually
		freed.
@param		jobmanager
			A pointer to the job manager from which to get
			the job function pointer.
@param		name
			The string data representing the name
			to retrieve. We must copy the data
			into a second character array in order
			to ensure the string is the same as the one stored.
@param		params
			An array of void pointers pointing to data
			that is to be used by the job.
			It is up to the job to cast/interpret this data
			appropriately.
@param		retval
			A pointer that is to be set with the return data.	
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_perform_job(
	sjm_t			*jobmanager,
	char			*name,
	void			**params,
	void			*retval
);

#ifdef  SJM_JSON_HANDLING
/**
@brief		Request the execution of a job from the job manager using
		a JSON array.
@details	Note, this assumes the json string is mutable in
		order to function properly. Any memory allocated on the
		heap during the job's execution must be manually freed.
@param		jobmanager
			A pointer to the job manager structure to use
			to execute the job.
@param		json
			A pointer to the JSON string (char array) to use.
			This must be mutable, as some bytes must
			temporarily be set to '\0' in order to properly
			parse the request.
			
			The JSON string represent a JSON array where
			the first element is the name of the job.
			All other parameters will either be interpreted
			as character arrays (strings) or integers and
			passed as parameters into the job's function
			call. Boolean literals, @c true and @c false,
			will be interpreted as the integers @c 1 and
			@c 0, respectively.
@param		returnval
			A pointer used to extract return data into.
			This is passed into the job function directly.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_request_job(
	sjm_t			*jobmanager,
	char			*json,
	void			*returnval
);
#endif

/**
@brief		Execute the next queued job.
@param		jobmanager
			The job manager that is scheduling the jobs
			from which to pick and execute the next queued job.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_execute_queued_job(
	sjm_t		*jobmanager
);

/**
@brief		This loops through all jobs and adds new jobs to the
		queue, if necessary.
@param		jobmanager
			The job manager that manages the scheduled jobs.
@returns	@c SJM_ERROR_OK on successes, an appropriate error code
		otherwise.
*/
sjm_error_t
sjm_queue_scheduled_jobs(
	sjm_t		*jobmanager
);

#ifdef  __cplusplus
}
#endif

#endif
