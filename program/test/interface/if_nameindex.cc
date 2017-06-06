#include <net/if.h>
#include <stdio.h>

int main() {
	struct if_nameindex *ni, *p;

	ni = if_nameindex();
	p = ni;

	while(p->if_index) {
		printf("%02u : %s\n", p->if_index, p->if_name);
		++p;
	}

	if_freenameindex(ni);
	return 0;
}
