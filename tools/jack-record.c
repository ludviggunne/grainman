#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>

#include <jack/jack.h>
#include <sndfile.h>

static const char *output_path = "jack-record-output.wav";
static jack_client_t   *client       = NULL;
static int              is_activated = 0;
static jack_port_t     *port         = NULL;
static int              is_connected = 0;
static char            *source_port  = NULL;
static pthread_mutex_t  cb_lock      = PTHREAD_MUTEX_INITIALIZER; /* For process callback   */
static pthread_mutex_t  cv_lock      = PTHREAD_MUTEX_INITIALIZER; /* For condition variable */
static pthread_cond_t   cv           = PTHREAD_COND_INITIALIZER;
static SNDFILE         *file         = NULL;
static unsigned int     sample_rate  = 0;

static void cleanup(void)
{
  pthread_mutex_lock(&cb_lock);

  if (is_connected)
    jack_disconnect(client, source_port, jack_port_name(port));

  free(source_port);

  if (port)
    jack_port_unregister(client, port);

  if (is_activated)
    jack_deactivate(client);

  if (client)
    jack_client_close(client);

  if (file) {
    sf_close(file);
    printf("%s saved\n", output_path);
  }

  pthread_mutex_unlock(&cb_lock);
  pthread_cond_signal(&cv);
}

static int srate_callback(jack_nframes_t new_sample_rate, void *args)
{
  (void) args;
  if (sample_rate != new_sample_rate) {
    fprintf(stderr, "Error: Sample rate changed, aborting...\n");
    exit(1);
  }

  sample_rate = new_sample_rate;
  return 0;
}

static int process_callback(jack_nframes_t nframes, void *args)
{
  (void) args;
  pthread_mutex_lock(&cb_lock);

  void *buffer = jack_port_get_buffer(port, nframes);
  sf_write_float(file, buffer, nframes);

  pthread_mutex_unlock(&cb_lock);
  return 0;
}

int main(int argc, char **argv)
{
  atexit(cleanup);

  int opt;
  int list_ports = 0;
  const char *port_pattern;

  while ((opt = getopt(argc, argv, "o:l")) != -1) {
    switch (opt) {
    case 'o':
      output_path = optarg;
      break;
    case 'l':
      list_ports = 1;
      break;
    default:
      fprintf(stderr, "Usage: %s [-o <output>] <port pattern>\n", argv[0]);
      exit(1);
    }
  }

  port_pattern = argv[optind];
  if (port_pattern == NULL && !list_ports) {
      fprintf(stderr, "Missing port pattern\n");
      fprintf(stderr, "Usage: %s [-o <output>] [ -l ] <port pattern>\n", argv[0]);
      exit(1);
  }

  client = jack_client_open("jack-record", 0, NULL);
  if (client == NULL) {
    fprintf(stderr, "Error: Failed to open JACK client\n");
    exit(1);
  }

  port = jack_port_register(client, "input", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (port == NULL) {
    fprintf(stderr, "Error: Failed to register port\n");
    exit(1);
  }

  const char **ports = jack_get_ports(client, list_ports ? NULL : port_pattern,
                                      JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
  if (ports == NULL || ports[0] == NULL) {
    fprintf(stderr, "Error: No ports matching pattern '%s'\n", port_pattern);
    exit(1);
  }

  if (list_ports) {
    for (const char **ptr = ports; *ptr; ++ptr) {
      printf("%s\n", *ptr);
    }
    exit(0);
  }

  int index;

  if (ports[1]) {
    printf("Multiple ports matching pattern '%s':\n", port_pattern);

    int counter = 1;
    for (const char **ptr = ports; *ptr; counter++, ptr++) {
      printf("    %d: %s\n", counter, *ptr);
    }

    char *line = NULL;
    size_t size = 0;

    for (;;) {
      printf("Selected port: ");
      if (getline(&line, &size, stdin) < 0) {
        fprintf(stderr, "Aborted\n");
        exit(1);
      }

      index = atoi(line);
      
      if (index > 0 && index <= counter) {
        index--;
        break;
      }
    }
  } else {
    index = 0;
  }

  source_port = strdup(ports[index]);
  jack_free(ports);

  printf("Connected to port %s\n", source_port);

  if (jack_connect(client, source_port, jack_port_name(port)) < 0) {
    fprintf(stderr, "Error: Failed to connect to port %s\n", source_port);
    exit(1);
  }
  is_connected = 1;

  sample_rate = jack_get_sample_rate(client);
  jack_set_sample_rate_callback(client, srate_callback, NULL);
  jack_set_process_callback(client, process_callback, NULL);

  SF_INFO info;
  info.channels = 1;
  info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
  info.samplerate = sample_rate;

  file = sf_open(output_path, SFM_WRITE, &info);
  if (file == NULL) {
    fprintf(stderr, "Error: Failed to open output file %s\n", output_path);
    exit(1);
  }

  if (jack_activate(client) != 0) {
    fprintf(stderr, "Error: Failed to activate client\n");
    exit(1);
  }

  pthread_cond_wait(&cv, &cv_lock);
}
