/******************************************************************************/
/**
@file
@author		Graeme Douglas, Scott Fazackerley
@see		For more information, refer to @ref dictionary.c.
*/
/******************************************************************************/

#include "dictionary.h"
#include <string.h>

ion_dictionary_compare_t
dictionary_switch_compare(
	key_type_t key_type
)
{
	ion_dictionary_compare_t compare;
	switch (key_type)
	{
		case key_type_numeric_signed:
		{
			compare = dictionary_compare_signed_value;
			break;
		}
		case key_type_numeric_unsigned:
		{
			compare = dictionary_compare_unsigned_value;
			break;
		}
		case key_type_char_array:
		{
			compare = dictionary_compare_char_array;
			break;
		}
		case key_type_null_terminated_string:
		{
				compare	= dictionary_compare_null_terminated_string;
				break;
		}
		default:
		{
			//do something - you must bind the correct comparison function
			break;
		}
	}

	return compare;
}

status_t
dictionary_create(
	dictionary_handler_t 	*handler,
	dictionary_t 			*dictionary,
	ion_dictionary_id_t 	id,
	key_type_t				key_type,
	int 					key_size,
	int 					value_size,
	int 					dictionary_size
)
{
	err_t err;
	ion_dictionary_compare_t compare = dictionary_switch_compare(key_type);

	err = handler->create_dictionary(id, key_type, key_size, value_size, dictionary_size, compare, handler, dictionary);
	if (err_ok == err)
	{
		dictionary->instance->id = id;
	}

	return err;
}

//inserts a record into the dictionary
//each dictionary will have a specific handler?
status_t
dictionary_insert(
	dictionary_t 				*dictionary,
	ion_key_t 					key,
	ion_value_t 				value
)
{
	return dictionary->handler->insert(dictionary, key, value);
}

status_t
dictionary_get(
	dictionary_t 				*dictionary,
	ion_key_t 					key,
	ion_value_t 				value
)
{
	return dictionary->handler->get(dictionary, key, value);
}
status_t
dictionary_update(
	dictionary_t 			*dictionary,
	ion_key_t 				key,
	ion_value_t 			value
)
{
	return dictionary->handler->update(dictionary, key, value);
}

status_t
dictionary_delete_dictionary(
	dictionary_t		*dictionary
)
{
	return dictionary->handler->delete_dictionary(dictionary);
}

status_t
dictionary_delete(
	dictionary_t		*dictionary,
	ion_key_t			key
)
{
	return dictionary->handler->remove(dictionary,key);
}

char
dictionary_compare_unsigned_value(
	ion_key_t 		first_key,
	ion_key_t		second_key,
	ion_key_size_t	key_size
)
{
	int idx;
	char return_value;
	/**
	 * In this case, the endiannes of the process does matter as the code does
	 * a direct comparison of bytes in memory starting for MSB.
	 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	for (idx = key_size - 1; idx >= 0; idx--)
	{
		if ((return_value = ((*(first_key+idx) > *(second_key+idx)) - (*(first_key+idx) < *(second_key+idx)))) != ZERO)
		{
			return return_value;
		}
	}
	return return_value;
#else
	/** @TODO This is a potential issue and needs to be tested on SAMD3 */
	for (idx = 0 ; idx < key_size ; idx++)
	{
		if ((return_value = ((*(first_key+idx) > *(second_key+idx)) - (*(first_key+idx) < *(second_key+idx)))) != ZERO)
		{
			return return_value;
		}
	}
	return return_value;
#endif
}

char
dictionary_compare_signed_value(
	ion_key_t 		first_key,
	ion_key_t		second_key,
	ion_key_size_t	key_size
	)
{
	int idx;
	char return_value;

	/**
	 * In this case, the endiannes of the process does matter as the code does
	 * a direct comparison of bytes in memory starting for MSB.
	 */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	//check the MSByte as signed
	idx = key_size - 1;						//Start at the MSB
#if DEBUG
	printf("key 1: %i key 2: %i \n",*(char *)(first_key+idx),*(char *)(second_key+idx));
#endif
	if ((return_value = ((*(char *)(first_key+idx) > *(char *)(second_key+idx)) - (*(char *)(first_key+idx) < *(char *)(second_key+idx)))) != ZERO)
		{
			return return_value;
		}
	//and then the rest as unsigned
	for (idx = key_size - 2; idx >= 0; idx--)
	{
		if ((return_value = ((*(first_key+idx) > *(second_key+idx)) - (*(first_key+idx) < *(second_key+idx)))) != ZERO)
		{
			return return_value;
		}
	}
	return return_value;
#else
	/** @TODO This is a potential issue and needs to be tested on SAMD3 */
	for (idx = 0 ; idx < key_size ; idx++)
	{
		if ((return_value = ((*(first_key+idx) > *(second_key+idx)) - (*(first_key+idx) < *(second_key+idx)))) != ZERO)
		{
			return return_value;
		}
	}
	return return_value;
#endif
}


char
dictionary_compare_char_array(
	ion_key_t 		first_key,
	ion_key_t		second_key,
	ion_key_size_t	key_size
)
{
	int result	= memcmp((char *)first_key, (char *)second_key, key_size);
	if (result > 0)
		return 1;
	if (result < 0)
		return -1;
	return 0;
}

char
dictionary_compare_null_terminated_string(
	ion_key_t 		first_key,
	ion_key_t		second_key,
	ion_key_size_t	key_size
)
{
	return strncmp((char *)first_key, (char *)second_key, key_size);
}

err_t
dictionary_open(
 	dictionary_handler_t 			*handler,
    dictionary_t 					*dictionary,
    ion_dictionary_config_info_t 	*config
)
{
	ion_dictionary_compare_t compare = dictionary_switch_compare(config->type);

	return handler->open_dictionary(handler, dictionary, config, compare);
}

err_t
dictionary_close(
    dictionary_t 					*dictionary
)
{
	return dictionary->handler->close_dictionary(dictionary);
}

err_t
dictionary_build_predicate(
	predicate_t				*predicate,
	predicate_type_t		type,
	...
)
{
	va_list arg_list;
	va_start (arg_list, type);

	predicate->type = type;

	switch(type)
	{
		case predicate_equality:
		{
			ion_key_t key = va_arg(arg_list, ion_key_t);
			predicate->statement.equality.equality_value = key;
			predicate->destroy = dictonary_destroy_predicate_equality;
			break;
		}
		case predicate_range:
		{

			ion_key_t lower_bound = va_arg(arg_list, ion_key_t);
			ion_key_t upper_bound = va_arg(arg_list, ion_key_t);

			predicate->statement.range.lower_bound = lower_bound;
			predicate->statement.range.upper_bound = upper_bound;
			predicate->destroy = dictonary_destroy_predicate_range;
			break;
		}
		case predicate_all_records:
		{

			predicate->destroy = dictionary_destroy_predicate_all_records;
			break;
		}
		case predicate_predicate:
		{
			/* TODO not implemented */
			return err_invalid_predicate;
		}
		default:
		{
			return err_invalid_predicate;
			break;
		}
	}

	va_end(arg_list);
	return err_ok;
}

void
dictonary_destroy_predicate_equality(
	predicate_t		**predicate
)
{
	if (*predicate != NULL)
	{
		free((*predicate)->statement.equality.equality_value);
		free(*predicate);
		*predicate = NULL;
	}
}

void
dictonary_destroy_predicate_range(
	predicate_t		**predicate
)
{
	if (*predicate != NULL)
	{
		free((*predicate)->statement.range.upper_bound);
		free((*predicate)->statement.range.lower_bound);
		free(*predicate);
		*predicate = NULL;
	}
}

void
dictionary_destroy_predicate_all_records(
	predicate_t 	**predicate
)
{
	if (*predicate != NULL)
	{
		free(*predicate);
		*predicate = NULL;
	}
}

err_t
dictionary_find(
	dictionary_t 	*dictionary,
	predicate_t 	*predicate,
	dict_cursor_t 	**cursor
)
{
	return dictionary->handler->find(dictionary, predicate, cursor);
}
