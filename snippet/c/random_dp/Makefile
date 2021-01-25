TARGETS = testVBMI

default: $(TARGETS)

%: %.cpp
	gcc -o $@ $^

clean:
	rm -f $(TARGETS)

.PHONY : all default
