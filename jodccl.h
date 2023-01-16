#ifndef INCLUDE_JODCCL_H
#define INCLUDE_JODCCL_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  JCL_PARSE_MISSING_VALUE = 0,
  JCL_PARSE_UNKNOWN_ARGUMENT = 1,
  JCL_PARSE_WRONG_TYPE = 2,
  JCL_PARSE_OK = 3,
  JCL_PARSE_MISSING_ARGUMENT = 4,
} JCLErrno;

typedef struct {
  JCLErrno err;
  size_t index;
} JCLError;

void jcl_add_bool(bool *orig, char *tag, char *short_tag, char *desc,
                  bool required);
void jcl_add_str(char **orig, char *tag, char *short_tag, char *desc,
                 bool required);
void jcl_add_float(float *orig, char *tag, char *short_tag, char *desc,
                   bool required);
void jcl_add_int(int *orig, char *tag, char *short_tag, char *desc,
                 bool required);
JCLError jcl_parse_args(char **argv);
void jcl_print_help(void);
void jcl_print_error(JCLError e);
#ifdef JODCCL_IMPLEMENTATION
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  JCL_INT,
  JCL_FLOAT,
  JCL_STRING,
  JCL_BOOL,
} JCLTypes;

typedef struct {
  char *short_tag;
  char *full_tag;
} JCLTag;

typedef struct {
  JCLTypes type;
  void *data;
  char *desc;
  JCLTag tag;
} JCLFlag;

#ifndef MAX_FLAGS // allows overriding
#define MAX_FLAGS 25
#endif // MAX_FLAGS

size_t num_flags = 0;

size_t num_required = 0;
size_t required_flags_indices[MAX_FLAGS];

JCLFlag flags[MAX_FLAGS];

static void jcl_add(JCLTypes type, void *data, char *tag, char *short_tag,
                    char *desc, bool required) {

  if (required)
    required_flags_indices[num_required++] = num_flags;

  JCLFlag *f = &flags[num_flags++];
  f->desc = desc;
  f->tag = (JCLTag){.short_tag = short_tag, .full_tag = tag};
  f->type = type;
  f->data = data;
}

#define ADD_ARG(type) jcl_add(type, orig, tag, short_tag, desc, required);

void jcl_add_bool(bool *orig, char *tag, char *short_tag, char *desc,
                  bool required) {
  ADD_ARG(JCL_BOOL);
}
void jcl_add_str(char **orig, char *tag, char *short_tag, char *desc,
                 bool required) {
  ADD_ARG(JCL_STRING);
}
void jcl_add_float(float *orig, char *tag, char *short_tag, char *desc,
                   bool required) {
  ADD_ARG(JCL_FLOAT);
}
void jcl_add_int(int *orig, char *tag, char *short_tag, char *desc,
                 bool required) {
  ADD_ARG(JCL_INT);
}
#define STREQ(str1, str2) strcmp(str1, str2) == 0
#define STRNCMP(str1, str2) strncmp(str1, str2, strlen(str1)) == 0

static int is_short_tag(char *tag) {
  size_t len = strlen(tag);
  if (len > 2) {
    if (strncmp(tag, "--", 2) == 0)
      return 0;
  }
  if (len > 1) {
    if (strncmp(tag, "-", 1) == 0)
      return 1;
  } else {
    return -1;
  }
  fprintf(stderr, "tag: %s", tag);
  exit(1);
}

static int find_relavent_flag(char *tag, bool is_short) {
  int index = -1;
  if (is_short) {
    tag += 1;
  } else {
    tag += 2;
  }

  for (size_t i = 0; i < num_flags; i++) {
    JCLFlag f = flags[i];
    char *cmp;

    if (is_short) {
      cmp = f.tag.short_tag;
    } else {
      cmp = f.tag.full_tag;
    }
    if (STRNCMP(cmp, tag)) {
      index = i;
      break;
    }
  }
  return index;
}

#define RET_ERROR(error, i)                                                    \
  return (JCLError) { .err = error, .index = i }

JCLError jcl_parse_args(char **argv) {
  size_t handled_indices[MAX_FLAGS];
  size_t counter = 0;
  argv += 1; // skip first argument as its the programs name
  char *arg;
  bool is_short;
  int index;
  while ((arg = *argv++) != NULL) {
    is_short = is_short_tag(arg);
    index = find_relavent_flag(arg, is_short);
    if (index < 0) {
      RET_ERROR(JCL_PARSE_UNKNOWN_ARGUMENT, counter + 1);
    }

    handled_indices[counter++] = (size_t)index;

    JCLFlag *f = &flags[index];

    char *matched_tag;
    if (is_short) {
      matched_tag = f->tag.short_tag;
    } else {
      matched_tag = f->tag.full_tag;
    }

    size_t matched_len = strlen(matched_tag);

    if (is_short)
      matched_len += 1;
    else
      matched_len += 2;

    char *parsed_arg = NULL;

    if (arg[matched_len] == '=') {
      parsed_arg = arg + matched_len + 1;
    } else if (f->type != JCL_BOOL) {
      parsed_arg = *argv++;
      if (parsed_arg == NULL) {
        RET_ERROR(JCL_PARSE_MISSING_VALUE, (size_t)index);
      }
    }
    switch (f->type) {
    case JCL_BOOL: {
      bool boolean;
      if (parsed_arg == NULL) { // true by default
        boolean = true;
      } else {
        if (STREQ(parsed_arg, "y"))
          boolean = true;
        else if (STREQ(parsed_arg, "t"))
          boolean = true;
        else if (STREQ(parsed_arg, "yes"))
          boolean = true;
        else if (STREQ(parsed_arg, "true"))
          boolean = true;

        else if (STREQ(parsed_arg, "n"))
          boolean = false;
        else if (STREQ(parsed_arg, "f"))
          boolean = false;
        else if (STREQ(parsed_arg, "no"))
          boolean = false;
        else if (STREQ(parsed_arg, "false"))
          boolean = false;
        else
          RET_ERROR(JCL_PARSE_WRONG_TYPE, (size_t)index);
      }
      *(bool *)f->data = boolean;
      break;
    }
    case JCL_FLOAT: {
      // yes im going to keep naming them like these lmao
      char *end_str;
      float floating_point = strtof(parsed_arg, &end_str);
      if (floating_point == 0 && end_str == parsed_arg) {
        RET_ERROR(JCL_PARSE_WRONG_TYPE, (size_t)index);
      }
      *(float *)f->data = floating_point;
      break;
    }
    case JCL_INT: {
      // test if its an integer
      char c;
      char *temp = parsed_arg;
      while ((c = *temp++) != '\0') {
        if (c < 0x30 || c > 0x39) // see if its in digit range
          RET_ERROR(JCL_PARSE_WRONG_TYPE, (size_t)index);
      }

      *(int *)f->data = atoi(parsed_arg);
      break;
    }
    case JCL_STRING: {
      *(char **)f->data = parsed_arg;
      break;
    }
    }
  }
  // check if all required flags were handlded
  for (size_t i = 0; i < num_required; i++) {
    size_t required_indice = required_flags_indices[i];
    bool found = false;
    for (size_t i = 0; i < counter; i++) {

      if (handled_indices[i] == required_indice)
        found = true;
    }
    if (!found) {
      RET_ERROR(JCL_PARSE_MISSING_ARGUMENT, required_indice);
    }
  }

  RET_ERROR(JCL_PARSE_OK, 0);
}
void jcl_print_help(void);

static const char *type_to_str(JCLTypes type) {
  switch (type) {
  case JCL_BOOL:
    return "bool";
  case JCL_FLOAT:
    return "float";
  case JCL_INT:
    return "int";
  case JCL_STRING:
    return "string";
  }
}

void jcl_print_error(JCLError err) {
  switch (err.err) {
  case JCL_PARSE_WRONG_TYPE: {
    JCLFlag f = flags[err.index];
    char *tag = f.tag.full_tag;
    const char *type = type_to_str(f.type);
    fprintf(stderr, "wrong type for %s, expected: %s \n", tag, type);
    break;
  }
  case JCL_PARSE_MISSING_ARGUMENT: {
    fprintf(stderr, "missing required argument: %s \n",
            flags[err.index].tag.full_tag);
    break;
  }
  case JCL_PARSE_MISSING_VALUE: {
    fprintf(stderr, "expected value for argument: %s \n",
            flags[err.index].tag.full_tag);
    break;
  }
  case JCL_PARSE_UNKNOWN_ARGUMENT: {
    fprintf(stderr, "unknown argument at position: %zu", err.index);
    break;
  }
  case JCL_PARSE_OK: {
    fprintf(stderr, "parsed successfully");
    break;
  }
  }
}

#endif // JODCCL_IMPLEMENTATION
#endif // INCLUDE JODCCL_H
