CXX := g++
CXXFLAGS := -std=c++11 -Wall -Wextra

TARGET := qcomm
RCG_TARGET := rcg

OBJDIR := obj

MODULES := main architecture noc circuit communication communication_time core gate mapping parameters statistics utils simulation command_line
OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(MODULES)))

RCG_MODULES := rcg circuit gate utils
RCG_OBJS := $(addprefix $(OBJDIR)/,$(addsuffix .o,$(RCG_MODULES)))

DEPS := $(OBJS:.o=.d)
RCG_DEPS := $(RCG_OBJS:.o=.d)

all: $(TARGET) $(RCG_TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(RCG_TARGET): $(RCG_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)
-include $(RCG_DEPS)

clean:
	rm -rf $(OBJDIR) $(TARGET) $(RCG_TARGET)

rebuild: clean all

.PHONY: all clean rebuild
