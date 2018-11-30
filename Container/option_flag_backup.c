	case 'C':
	    //TODO: The cpu shares weight to be set (cpu-cgroup controller)
	    // I'm not sure I got the dereferencing correct, so please check this!
	    // cgroups = array of pointers to cgroups_control structs
	    // settings = array of pointers to cgroup_setting structs
	    
	    // Here we enforce that the argument is an integer (should we? can it be a decimal?)
	    /*if (sscanf(optarg, "%d", (cgroups[2]->settings)[0]->value) != 1) {
		    fprintf(stderr, "CPU shares not as expected: %s\n", optarg);
		    cleanup_stuff(argv, sockets);
		    return EXIT_FAILURE;
	    }*/
	    //In the sample code (case 'm') there is no checking of the formate of optarg
	    //for the mount point, so I think for now at least I'm going to comment out all
	    //the checks I did and just plain store the string optarg as is.
	    
	    strcpy((cgroups[2]->settings)[0]->value, optarg);
	    break;
	case 's':
	    //TODO: The cpu cores to which the container must be restricted (cpuset-cgroup controller)
	    // Need to check that you can edit cpuset.cpus by echo <integer> or echo <range ints>
	    // Consider adding a restriction that if a range is entered, the first int must be
	    //     strictly less than the second
	    // Consider adding other restrictions based on valied CPU core identifiers
	    // Consider changing input format to allow list of valid CPUs to be entered (alternative:
	    //     use -s flag multiple times for each list entry)
	    // See man cpuset for more info
	    /* if (sscanf(optarg, "%d-%d", (cgroups[3]->settings)[0]->value) != 1) {
		    if (sscanf(optarg, "%d", (cgroups[3]->settings)[0]->value) != 1) {
                            fprintf(stderr, "CPU cores not as expected: %s\n", optarg);
                            cleanup_stuff(argv, sockets);
                            return EXIT_FAILURE;
	            }
            }*/
	    strcpy((cgroups[3]->settings)[0]->value, optarg);
	    break;
	case 'p':
	    //TODO: The max number of processes allowed within a container (pid-cgroup controller)
	     /*if (sscanf(optarg, "%d", (cgroups[4]->settings)[0]->value) != 1) {
                    fprintf(stderr, "Max processes not as expected: %s\n", optarg);
                    cleanup_stuff(argv, sockets);
                    return EXIT_FAILURE;
            }*/
	    strcpy((cgroups[4]->settings)[0]->value, optarg);
	    break;
	case 'M':
	    //TODO: The memory consumption allowed in the container (memory-cgroup controller)
	    /*if (sscanf(optarg, "%d", (cgroups[1]->settings)[0]->value) != 1) {
                    fprintf(stderr, "Memory not as expected: %s\n", optarg);
                    cleanup_stuff(argv, sockets);
                    return EXIT_FAILURE;
            }*/
	    strcpy((cgroups[1]->settings)[0]->value, optarg);
	    break;
	case 'r':
	    //TODO: The read IO rate in bytes (blkio-cgroup controller)
	    //Q: Should we accept a single rate (integer) for all devices? Or should we
	    //   expect 'Major:Minor Bytes' format?
	    /*int major, minor, bytes;
	    if (sscanf(optarg, "%d:%d %d", &major, &minor, &bytes) != 1) {
                    fprintf(stderr, "Read IO rate not as expected: %s\n", optarg);
                    cleanup_stuff(argv, sockets);
                    return EXIT_FAILURE;
            } else {
		    char *val = 
		    (cgroups[0]->settings)[0]->value = 
	    }*/
	    strcpy((cgroups[0]->settings)[0]->value, optarg);
	    break;
	case 'w':
	    //TODO: The write IO rate in bytes (blkio-cgroup controller)
	    /*if (sscanf(optarg, "%d:%d %d", (cgroups[0]->settings)[1]->value) != 1) {
                    fprintf(stderr, "Write IO rate not as expected: %s\n", optarg);
                    cleanup_stuff(argv, sockets);
                    return EXIT_FAILURE;
            }*/
	    strcpy((cgroups[0]->settings)[1]->value, optarg);
	    break;
	case 'H':
	    config.hostname = optarg;
	    break;
        default:
            cleanup_stuff(argv, sockets);
            return EXIT_FAILURE;
        }
        last_optind = optind;
    }