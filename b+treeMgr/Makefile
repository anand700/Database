all: clean exec

exec: assignment4
	./assignment4

assignment4: test_assign4_1.o dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o expr.o btree_mgr.o
	gcc test_assign4_1.o dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o expr.o btree_mgr.o -o assignment4

test_assign4_1.o: test_assign4_1.c test_helper.h dberror.h expr.h btree_mgr.h tables.h
	gcc -c test_assign4_1.c

buffer_mgr.o: buffer_mgr.c dt.h dberror.h storage_mgr.h buffer_mgr_stat.h buffer_mgr.h
	gcc -c buffer_mgr.c

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr.h buffer_mgr_stat.h
	gcc -c buffer_mgr_stat.c

storage_mgr.o: storage_mgr.c dberror.h storage_mgr.h
	gcc -c storage_mgr.c

dberror.o: dberror.c dberror.h
	gcc -c dberror.c

btree_mgr.o: btree_mgr.c storage_mgr.h buffer_mgr.h buffer_mgr_stat.h dberror.h dt.h tables.h record_mgr.h btree_mgr.h
	gcc -c btree_mgr.c

clean:
	rm -rf *.o
