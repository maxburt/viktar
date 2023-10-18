//Written by Max Burt
//10/18/23
//Viktar program, similar to tar command
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utime.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include "viktar.h"

#define DEFAULT_FILE_MODE 0644
#define BUFFER_SIZE 1024

#define min(a, b) ((unsigned long)(a) < (unsigned long)(b) ? (a) : (b))

void print_help(void);

//util function to set file attributes after extracting them from archive
int set_file_attributes(const char * filename, const viktar_header_t * header);

//util functions for printing inode info
//for read_table_long
void print_file_permission(mode_t mode);
void print_user_name(uid_t uid);
void print_group_name(gid_t gid);
void print_time(struct timespec st_tim);
const char * get_time_zone(void);
void print_header_info(viktar_header_t header);

//the functions that get triggered based on flags passed, (-c, -x -t, -T)
//the from_file versions are made to handle < > redirecion instead of -f flag
int create_archive(const char * archive_file, char * members[], int member_count);
int extract_archive(const char * archive_file, char * members[], int member_count);
int read_table_short(const char * archive_file);
int read_table_long(const char * archive_file);

static int verbose = 0;				//global so all functions can see if output should be verbose

int main(int argc, char *argv[]) {
    	int opt;
   	char * archive_file_name = NULL;	
    	char * members[argc];			//stores members passed at command line
  	int member_count = 0;
	int flag = 0; // 1 = c, 2 = x, 3 = t, 4 = T	//number to flag translation
	
	while ((opt = getopt(argc, argv, OPTIONS)) != -1) {	// Process command-line options using getopt
		switch (opt) {
			case 'c':
				flag = 1;
				break;
			case 'x':
                		flag = 2;
                		break;
            		case 't':
                		flag = 3;
                		break;
            		case 'T':
                		flag = 4;
                		break;
            		case 'f':
                		archive_file_name = optarg;
                		break;
            		case 'h':
                		print_help();
                		return EXIT_SUCCESS;
            		case 'v':
                		verbose = 1;
				fprintf(stderr, "verbose level: %d\n", verbose);
                		break;
            		default:
                		fprintf(stderr, "Warning: Invalid option passed. Run with -h to see valid options\n");
				break;
        		}
    	}
	
	while (optind < argc) {			//fill members array with argumens passed at command line
		members[member_count] = argv[optind];
		member_count++;
        	optind++;
    	}	

	if (flag == 1) {
		create_archive(archive_file_name, members, member_count);
	} else if (flag == 2) {
		extract_archive(archive_file_name, members, member_count);
	} else if (flag == 3) {
		read_table_short(archive_file_name);
	} else if (flag == 4) {
		read_table_long(archive_file_name);
	} else {
		fprintf(stderr, "You must specify one of -c, -x, -t, or -T\n");
	}
	return EXIT_SUCCESS;
}

void print_help(void) {
        printf("help text\n");
        printf("\t./viktar\n");
        printf("\tOptions: xctTf:hv\n");
        printf("\t\t-x\t\textract file/files from archive\n");
        printf("\t\t-c\t\tcreate an archive file\n");
        printf("\t\t-t\t\tdisplay a short table of contents of the archive file\n");
        printf("\t\t-T\t\tdisplay a long table of contents of the archive file\n");
        printf("\t\tOnly one of xctT can be specified\n");
        printf("\t\t-f filename\tuse filename as the archive file\n");
        printf("\t\t-v\t\tgive verbose diagnostic messages\n");
        printf("\t\t-h\t\tdisplay this AMAZING help message\n");
}

int create_archive(const char *archive_file, char *members[], int member_count) {
	int archive_fd;
    	viktar_header_t header;
    	int idx;
        mode_t old_mode = 0;
        old_mode = umask(0);

	if (verbose == 1) {
		if (archive_file != NULL) fprintf(stderr, "creating archive file: \"%s\"\n", archive_file);
		else fprintf(stderr, "archive output to stdout\n");
	}

	if (archive_file != NULL) {
		archive_fd = open(archive_file, O_WRONLY | O_CREAT | O_TRUNC, DEFAULT_FILE_MODE);
	}
	else {
		archive_fd = 1; //set it to stdout
	}

	if (archive_fd == -1) {
        	perror("Failed to create archive file");
		exit(EXIT_FAILURE);
    	}

	// Write the viktar header to the archive
   	write(archive_fd, VIKTAR_FILE, strlen(VIKTAR_FILE));
	
	//iterate over all members to be added to archive
	for (idx = 0; idx < member_count; ++idx) {
		int member_fd;
        	struct stat member_info;
        	char buffer[BUFFER_SIZE];
        	ssize_t bytes_read;
		if (verbose == 1) {
			fprintf(stderr, "\tadding file: %s to archive: %s\n", members[idx], archive_file);				}

		// Open the member file for reading
        	member_fd = open(members[idx], O_RDONLY);
        	
		if (member_fd == -1) {
            		perror("Failed to open member file");
            		exit(EXIT_FAILURE);
        	}

		// use fstat to get info about file, store it in member_info
		if (fstat(member_fd, &member_info) == -1) {
            		perror("Failed to get member file information");
            		exit(EXIT_FAILURE);
        	}
			
		//write a viktar_header to the file
		strncpy(header.viktar_name, members[idx], VIKTAR_MAX_FILE_NAME_LEN);
		header.st_mode = member_info.st_mode;
        	header.st_uid = member_info.st_uid;
        	header.st_gid = member_info.st_gid;
        	header.st_size = member_info.st_size;
        	header.st_atim = member_info.st_atim;
       	 	header.st_mtim = member_info.st_mtim;
        	header.st_ctim = member_info.st_ctim;

		// Write the viktar header to the archive
        	write(archive_fd, &header, sizeof(viktar_header_t));

		// Read and write the member file's content to the archive
        	while ((bytes_read = read(member_fd, buffer, BUFFER_SIZE)) > 0) {
            		write(archive_fd, buffer, bytes_read);
        	}

        	// Close the member file
        	close(member_fd);
    	}
    	
	// Close the archive file
	close(archive_fd);
	umask(old_mode);

	return EXIT_SUCCESS;
}

int extract_archive(const char *archive_file, char *members[], int member_count) {
	int archive_fd;
    	viktar_header_t header;
    	char buffer[BUFSIZ];
	char viktar_check[strlen(VIKTAR_FILE)];
    	
	if (archive_file != NULL) {
		archive_fd = open(archive_file, O_RDONLY);
	}	
	else {
		archive_fd = 0;		//set file descriptor to '0' for stdin
	}
	
	if (archive_file == NULL) fprintf(stderr, "archive from stdin\n");	
    	
	if (verbose == 1 && archive_file != NULL) fprintf(stderr, "reading archive file: \"%s\"\n", archive_file);

	if (archive_fd == -1) {
        	perror("Failed to open archive file");
        	exit(EXIT_FAILURE);
    	}
    	
	if (read(archive_fd, viktar_check, sizeof(viktar_check)) != (int)sizeof(viktar_check) || strcmp(viktar_check, VIKTAR_FILE) != 0) {
        	fprintf(stderr, "Invalid archive format\n");
        	close(archive_fd);
		exit(EXIT_FAILURE);
    	}

	while (1) {
		//this is a flag that determines if the member should be extracted or not
		int extract_member = 0;

		//read header info and store it in header variable
		ssize_t header_read = read(archive_fd, &header, sizeof(viktar_header_t));

		if (header_read == 0) {
        		// End of the archive file, break the loop
        		break;
    		}
		else if (header_read != sizeof(viktar_header_t)) {
        		fprintf(stderr, "Failed to read member header\n");
        		close(archive_fd);
			exit(EXIT_FAILURE);
    		}
		
		if (member_count == 0) {  //no files were passed at command line, therefore all files should be extracted
			extract_member = 1;
		}
		else { //file names were passed at command line and we should only extract those files

 			//check if header's file name is one that should be extracted
                	//i.e. one that was passed at the command line
                	for (int j = 0; j < member_count; j++) {
                        	if (strcmp(header.viktar_name, members[j]) == 0) {
                                	extract_member = 1;
                                	break;
                        	}
                	}
		
		}		
		
		// If the member is to be extracted, create the member file
        	if (extract_member) {
			off_t bytes_to_read = header.st_size;
			int member_fd = open(header.viktar_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);

			if (verbose == 1) fprintf(stderr, "\textracting file: %s from archive: %s\n", header.viktar_name, archive_file);
			if (member_fd == -1) {
        			perror("Failed to create member file");
        			close(archive_fd);
				exit(EXIT_FAILURE);
			}
    			while (bytes_to_read > 0) {
				ssize_t bytes_read;
				ssize_t write_result;
				if ((size_t)bytes_to_read < sizeof(buffer)) {
    					bytes_read = read(archive_fd, buffer, (size_t)bytes_to_read);
				} else {
    					bytes_read = read(archive_fd, buffer, sizeof(buffer));
				}
        			if (bytes_read <= 0) {
            				fprintf(stderr, "Failed to read member content from archive\n");
            				close(archive_fd);
            				close(member_fd);
            				return 1;
        			}

				write_result = write(member_fd, buffer, bytes_read);
        			if (write_result < 0) {
            				perror("Failed to write member content to file");
            				close(archive_fd);
            				close(member_fd);
            				return 1;
        				}

		        	bytes_to_read -= write_result;

    			}

			// Close the member file
			close(member_fd);
			
			//set file attriutes of newly created file  to match viktar_header_t 
			set_file_attributes(header.viktar_name, &header);
		} else {
            		// Skip the member content
    			if (lseek(archive_fd, header.st_size, SEEK_CUR) == -1) {
        			perror("Failed to skip member content");
        			close(archive_fd);
        			exit(EXIT_FAILURE);
    			}
            	}
        }
    	close(archive_fd);
	return EXIT_SUCCESS;
}

int set_file_attributes(const char * filename, const viktar_header_t * header) {
	struct stat st;
     	struct utimbuf times;
	mode_t old_mode = 0;
        old_mode = umask(0);
	
    	// Get the current file attributes
    	if (stat(filename, &st) == -1) {
        	perror("Failed to get file attributes");
		exit(EXIT_FAILURE);
    	}

    	// Set the attributes to match the header
    	st.st_mode = header->st_mode;
    	st.st_uid = header->st_uid;
    	st.st_gid = header->st_gid;
    	st.st_size = header->st_size;

    	// Convert struct timespec to time_t for utime
    	times.actime = header->st_atim.tv_sec;
    	times.modtime = header->st_mtim.tv_sec;

    	// Set the times using utime
    	if (utime(filename, &times) == -1) {
        	perror("Failed to set file times");
		exit(EXIT_FAILURE);
    	}

    	// Set the updated attributes to the file
    	if (chmod(filename, st.st_mode) == -1) {
        	perror("Failed to set file attributes");
		exit(EXIT_FAILURE);
    	}
	umask(old_mode);
   	return EXIT_SUCCESS;;
}

int read_table_short(const char * archive_file) {
	int archive_fd;
        viktar_header_t header;
        char viktar_check[strlen(VIKTAR_FILE)];
	ssize_t result;
	ssize_t bytes_read;;
	if (archive_file != NULL) archive_fd = open(archive_file, O_RDONLY);
	else archive_fd = 0;	//set fd to stdin (0)
	
	if (archive_file == NULL) fprintf(stderr, "archive from stdin\n");

	if (verbose == 1 && archive_file != NULL) {
		fprintf(stderr, "reading archive file: \"%s\"\n", archive_file);
	}

        if (archive_fd == -1) {
                perror("Failed to open archive file");
               	exit(EXIT_FAILURE);
        }

	result = read(archive_fd, viktar_check, sizeof(viktar_check));

	//null terminate the string to prevent overflow...
	viktar_check[result] = '\0';
	
	if (result < 0) {
    		perror("Error reading from file");
		close(archive_fd);
                exit(EXIT_FAILURE);
	} else if (result == 0) {
    		printf("File is empty, therefore not a proper viktar file\n");
		close(archive_fd);
                exit(EXIT_FAILURE);
	} else {
    		bytes_read = result;
	}

        if (bytes_read != (int)sizeof(viktar_check) || strcmp(viktar_check, VIKTAR_FILE) != 0) {

                fprintf(stderr, "Invalid archive format\n");
                close(archive_fd);
                exit(EXIT_FAILURE);
        }
	if (archive_file == NULL) fprintf(stderr, "Contents of viktar file: \"stdin\"\n");
	else fprintf(stderr, "Contents of viktar file: \"%s\"\n", archive_file);

	while (1) {
		//read header info and store it in header variable
                ssize_t header_read = read(archive_fd, &header, sizeof(viktar_header_t));

		if (header_read == 0) {	//reached end of archive file
			break;
		}
		else if (header_read != sizeof(viktar_header_t)) {
                        fprintf(stderr, "Failed to read member header\n");
                        close(archive_fd);
			exit(EXIT_FAILURE);
                }
		printf("\tfile name: %s\n", header.viktar_name);

		// Skip the files actual  content
                if (lseek(archive_fd, header.st_size, SEEK_CUR) == -1) {
                	perror("Failed to skip member content");
                        close(archive_fd);
                        exit(EXIT_FAILURE);
                }
	}
	close(archive_fd);
	return EXIT_SUCCESS;
}

//util function
///prints file permissions
void print_file_permission(mode_t mode) {
	// Extract the file type
    	mode_t file_type = mode & S_IFMT;

    	// Determine and print the file type
    	if (file_type == S_IFREG) {
        	printf("-");
    	} else if (file_type == S_IFDIR) {
        	printf("d");
    	} else if (file_type == S_IFLNK) {
        	printf("l");
    	} else {
        	// Handle other file types as needed
        	printf("?");
    	}
    	printf((mode & S_IRUSR) ? "r" : "-");
    	printf((mode & S_IWUSR) ? "w" : "-");
    	printf((mode & S_IXUSR) ? "x" : "-");
    	printf((mode & S_IRGRP) ? "r" : "-");
    	printf((mode & S_IWGRP) ? "w" : "-");
    	printf((mode & S_IXGRP) ? "x" : "-");
    	printf((mode & S_IROTH) ? "r" : "-");
    	printf((mode & S_IWOTH) ? "w" : "-");
    	printf((mode & S_IXOTH) ? "x" : "-");
    	printf("\n");
}

//util function prints UserName
void print_user_name(uid_t uid) {
	struct passwd *pwd;
    	pwd = getpwuid(uid);
    	if (pwd == NULL) {
        	perror("getpwuid");
    	} else {
        	printf("%s\n", pwd->pw_name);
    	}
}

void print_group_name(gid_t gid) {
	struct group *grp;
    	grp = getgrgid(gid);
    	if (grp == NULL) {
        	perror("getgrgid");
    	} else {
        	printf("%s\n", grp->gr_name);
    	}
}

void print_time(struct timespec st_tim) {
    struct tm timeInfo;
    char mtimeStr[32]; // Ensure enough space for the formatted string

    time_t mtime = st_tim.tv_sec;
    localtime_r(&mtime, &timeInfo);

    strftime(mtimeStr, sizeof(mtimeStr), "%Y-%m-%d %H:%M:%S", &timeInfo);

    printf("%s %s\n", mtimeStr, get_time_zone());
}

// Function to get the timezone abbreviation for the current time
const char * get_time_zone(void) {
    time_t now;
    struct tm tm_info;
    char buffer[6];

    time(&now);
    if (localtime_r(&now, &tm_info) == NULL) {
        perror("localtime");
        return "";
    }

    if (strftime(buffer, sizeof(buffer), "%Z", &tm_info) == 0) {
        return "";
    }

    return strdup(buffer);
}

void print_header_info(viktar_header_t header) {
	printf("\tfile name: %s\n", header.viktar_name);
	printf("\t\tmode:  ");
        print_file_permission(header.st_mode);
        printf("\t\tuser:  ");
        print_user_name(header.st_uid);
        printf("\t\tgroup: ");
        print_group_name(header.st_gid);
        printf("\t\tsize:  %ld\n", header.st_size);
        printf("\t\tmtime: ");
        print_time(header.st_mtim);
        printf("\t\tatime: ");
        print_time(header.st_atim);
}

int read_table_long(const char * archive_file) {
  	int archive_fd;
        viktar_header_t header;
        char viktar_check[strlen(VIKTAR_FILE)];
        ssize_t result;
        ssize_t bytes_read;;
        
	if (archive_file != NULL) archive_fd = open(archive_file, O_RDONLY);
	else archive_fd = 0;	//set fd to stdin (0)

	if (archive_file == NULL) fprintf(stderr, "archive from stdin\n");

        if (verbose == 1 && archive_file != NULL) {
                fprintf(stderr, "reading archive file: \"%s\"\n", archive_file);
        }

        if (archive_fd == -1) {
                perror("Failed to open archive file");
                exit(EXIT_FAILURE);
        }

        result = read(archive_fd, viktar_check, sizeof(viktar_check));

        //null terminate the string to prevent overflow...
        //i couldn't figure out why this was overflowing... but doing this helped
        viktar_check[result] = '\0';
        ///////////

        if (result < 0) {
                perror("Error reading from file");
                close(archive_fd);
                exit(EXIT_FAILURE);
        } else if (result == 0) {
                printf("File is empty, therefore not a proper viktar file\n");
                close(archive_fd);
                exit(EXIT_FAILURE);
        } else {
                bytes_read = result;
        }

        if (bytes_read != (int)sizeof(viktar_check) || strcmp(viktar_check, VIKTAR_FILE) != 0) {

                fprintf(stderr, "Invalid archive format\n");
                close(archive_fd);
                exit(EXIT_FAILURE);
        }

	if (archive_file != NULL) fprintf(stderr, "Contents of viktar file: \"%s\"\n", archive_file);
	else fprintf(stderr, "Contents of viktar file: \"stdin\"\n");

        while (1) {
                //read header info and store it in header variable
                ssize_t header_read = read(archive_fd, &header, sizeof(viktar_header_t));

                if (header_read == 0) { //reached end of archive file
                        break;
                }
                else if (header_read != sizeof(viktar_header_t)) {
                        fprintf(stderr, "Failed to read member header\n");
                        close(archive_fd);
                        exit(EXIT_FAILURE);
                }
		print_header_info(header);

                // Skip the files actual  content
                if (lseek(archive_fd, header.st_size, SEEK_CUR) == -1) {
                        perror("Failed to skip member content");
                        close(archive_fd);
                        exit(EXIT_FAILURE);
                }
        }
        close(archive_fd);
	return EXIT_SUCCESS;
}
