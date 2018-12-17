/* Shawn Hillstrom -- Program 1 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

FILE * fdopen(int fd, const char * mode);


/* struct: user
 * ------------
 * Stores access information for a user.
 * 
 * name: Name of the user.
 * perm: Permissions associated with user.
 */
typedef struct {
	char name[100];
	char perm;
} user;

/* function: changeEUID
 * -------------------
 * Sets a new EUID.
 *
 * newUID: New UID value.
 *
 * returns: 1 on success, 0 on failure.
 */
int changeEUID(uid_t newUID) {
	if (seteuid(newUID) < 0) {
		//printf("Unable to set EUID.\n");
		return 0;
	}
	return 1;
}

/* function: readAccess
 * --------------------
 * Read access control file to determine user permissions.
 *
 * accessfp: Access control file pointer.
 * userPerms: Array of type user containing all entries read from access control file.
 *
 * returns: Number of read entries or -1 on failure.
 */
int readAccess(FILE * accessfp, user * userPerms) {

	// Initialization.
	int length = 0;
	char buffer[100]; // Buffer set to 100 characters.

	while (fgets(buffer, 100, accessfp)) {
		if (buffer[0] != '#') {
			char nameBuf[100];
			char readPerm;
			char garbage[100];
			if (sscanf(buffer, "%s %c %s", nameBuf, &readPerm, garbage) != 2) {
				//printf("Malformed entry detected\n");
				return -1;
			} else {
				if (readPerm == 'r' || readPerm == 'w' || readPerm == 'b') {
					strcpy(userPerms[length].name, nameBuf);
					userPerms[length].perm = readPerm;
					length++;
				} else {
					//printf("Malformed entry detected\n");
					return -1;
				}
			}
		}
	}

	return length;

}

/* function: checkPerms
 * --------------------
 * Check through system permissions and compare with permissions in access control file.
 *
 * euid: EUID value.
 * userPerms: List of read permissions from access control file.
 * length: Number of items in userPerms.
 * ourUser: Pointer to user where user data (if found) is stored.
 * 
 * returns: void
 */
int checkPerms(uid_t ruid, user * userPerms, int length, user * ourUser) {

	struct passwd * userInfo = getpwuid(ruid); // Get EUID's user information.

	for (int i = 0; i < length; i++) {
		if (strcmp(userInfo->pw_name, userPerms[i].name) == 0) {
			strcpy(ourUser->name, userPerms[i].name);
			ourUser->perm = userPerms[i].perm;
			return 1;
		}
	}

	return 0;

}

/* function: checkDest
 * -------------------
 * Checks for the existance of a file destination and ask for user permission to proceed 
 *	if found.
 *
 * dest: Path for destination file.
 *
 * returns: 1 if file does not exist of user chooses to proceed, 0 otherwise.
 */
int checkDest(char * dest) {
	if (access(dest, F_OK) < 0) {
		return 1;
	} else {
		char overwrite = 0;
		//printf("Do you wish to overwrite %s? [Y/N]\n", dest);
		scanf("%c", &overwrite);
		if (overwrite != 'Y') {
			//printf("User chose not to proceed\n");
			return 0;
		}
		remove(dest);
		return 1;
	}
}

/* function: copyToDest
 * --------------------
 * Copies source file to destination file.
 *
 * srcfp: File pointer to source.
 * destfp: File pointer to destination.
 *
 * returns: void.
 */
void copyToDest(FILE * srcfp, FILE * destfp) {

	fseek(srcfp, 0L, SEEK_END);
	int endPos = ftell(srcfp);
	fseek(srcfp, 0L, SEEK_SET);

	while (endPos--) {
		char readChar = fgetc(srcfp);
		fputc(readChar, destfp);
	}

}

/* function: assignPerms
 * ---------------------
 * Assign permissions to destination file.
 *
 * dest: Destination file.
 * user: user with specified permissions to assign.
 *
 * returns: void
 */
void assignPerms(char * dest, user * ourUser) {
	if (ourUser->perm == 'r') {
		chmod(dest, S_IRUSR);
	} else if (ourUser->perm == 'w') {
		chmod(dest, S_IWUSR);
	} else {
		chmod(dest, (S_IRUSR | S_IWUSR));
	}
}

/* Main Function */
int main(int argc, char *argv[]) {

/* --INITIALIZATION-- */

	// Record RUID (Admin) and EUID (User).
	uid_t ruid = getuid();
	uid_t euid = geteuid();

	// Restore EUID, exit if unable to.
	if (!changeEUID(ruid)) return 1;

	// Exit immediately if get does not receive correct amount of arguments.
	if (argc != 3) {
		//printf("Not enough arguments\n");
		return 1; 
	}

	// Record arguments to main.
	char * src = argv[1]; // source
	char acs[strlen(src)+7]; // source.access
	strcpy(acs, src);
	strcat(acs, ".access");
	char * dest = argv[2]; // destination

/* --OPEN RESTRICTED FILES READ ONLY-- */

	// Enter EUID mode, exit if unable to.
	if (!changeEUID(euid)) return 1;

	// Open restricted files.
	int srcfd = open(src, O_RDONLY);
	int accessfd = open(acs, O_RDONLY);

	// Restore EUID, exit if unable to.
	if (!changeEUID(ruid)) return 1;

	// Exit if unable to read files.
	if (srcfd < 0 || accessfd < 0) {
		//printf("Could not open %s or %s.\n", src, acs);
		return 1;
	}

/* --GET FILE STATS AND CHECK-- */

	// Define structures for file stats.
	struct stat srcStat;
	struct stat accessStat;

	// Get file stats or exit if unable to.
	if (fstat(srcfd, &srcStat) < 0 || fstat(accessfd, &accessStat) < 0) {
		//printf("Could not acquire stats for %s or %s.\n", src, acs);
		return 1;
	}

	// Exit if source is not a regular file.
	if (!S_ISREG(srcStat.st_mode)) {
		//printf("%s is not a regular file.\n", src);
		return 1;
	}

	// Exit if source.access is a symbolic link.
	if (S_ISLNK(accessStat.st_mode)) {
		//printf("%s is a symbolic link.\n", acs);
		return 1;
	}

	// Exit if the owner of source is not the owner of source.access.
	if (srcStat.st_uid != accessStat.st_uid) {
		//printf("%s and %s have different owners.\n", src, acs);
		return 1;
	}

	// Exit if group permissions are incorrect for source.access.
	if (accessStat.st_mode & S_IRGRP) {
		//printf("%s has incorrect group permissions.\n", acs);
		return 1;
	}

	// Exit if other permissions are incorrect for source.access.
	if (accessStat.st_mode & S_IROTH) {
		//printf("%s has incorrect other permissions.\n", acs);
		return 1;
	}

/* --READ AND PARSE ACCESS CONTROL FILE-- */

	FILE * accessfp = fdopen(accessfd, "r"); // File pointer for source.access.

	user userPerms[100]; // Array of 100 users.

	int length = readAccess(accessfp, userPerms); // Read source.access.

	// Exit if we do not have a regular access list.
	if (length <= 0) {
		//printf("%s does not have integrity.\n", acs);
		return 1;
	}

	fclose(accessfp); // Close file pointer for source.access.

/* --CHECK FOR USER PERMISSION-- */

	// User to store user data (if found).
	user ourUser;

	// Check permissions, exit if user does not have access.
	if (!checkPerms(ruid, userPerms, length, &ourUser)) {
		//printf("User does not have access\n");
		return 1;
	}

/* --CHECK FOR DESTINATION EXISTENCE-- */

	/* Check to see if user wants to overwrite file */
	if (!checkDest(dest)) {
		//printf("Cannot write to destination.\n");
		return 1;
	}

/* --COPY SOURCE TO DESTINATION-- */

	// Get file pointers.
	FILE * srcfp = fdopen(srcfd, "r");
	FILE * destfp = fopen(dest, "w"); // Should not open if user does not have permission to write.

	// Exit if destination cannot be opened for write.
	if (destfp == NULL) {
		//printf("Destination could not be written to\n");
		return 1;
	}

	copyToDest(srcfp, destfp); // Copy source to destination.

	// Close file pointers.
	fclose(srcfp);
	fclose(destfp);

	assignPerms(dest, &ourUser); // Assign permissions for destination file.

/* --SUCCESSFUL EXIT-- */

	// Close all files.
	close(srcfd);
	close(accessfd);

	//printf("Success!\n");

	return 0;
}
