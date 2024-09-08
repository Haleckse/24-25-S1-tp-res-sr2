CC  = gcc
SYS = -std=gnu11 -Wall
LIB = -lpthread

# ============================= #
NOM_ETU = NOM_PRENOM_A_CHANGER
# ============================= #

SRCDIR = src
OBJDIR = obj
BINDIR = bin
FILESDIR = fichiers

SENDER = $(BINDIR)/emetteur
RECEIVER = $(BINDIR)/recepteur

OBJ_COMMON = $(OBJDIR)/config.o $(OBJDIR)/services_reseau.o $(OBJDIR)/couche_transport.o
OBJ_APP_NC = $(OBJDIR)/appli_non_connectee.o

OBJ_TDD0_S = $(OBJDIR)/proto_tdd_v0_emetteur.o
OBJ_TDD0_R = $(OBJDIR)/proto_tdd_v0_recepteur.o

OBJ_TDD1_S = $(OBJDIR)/proto_tdd_v1_emetteur.o
OBJ_TDD1_R = $(OBJDIR)/proto_tdd_v1_recepteur.o

OBJ_TDD2_S = $(OBJDIR)/proto_tdd_v2_emetteur.o
OBJ_TDD2_R = $(OBJDIR)/proto_tdd_v2_recepteur.o

OBJ_TDD3.1_S = $(OBJDIR)/proto_tdd_v3.1_emetteur.o
OBJ_TDD3.2_S = $(OBJDIR)/proto_tdd_v3.2_emetteur.o
OBJ_TDD3.1_R = $(OBJDIR)/proto_tdd_v3_recepteur.o
OBJ_TDD3.2_R = $(OBJDIR)/proto_tdd_v3_recepteur.o

OBJ_TDD4_S = $(OBJDIR)/proto_tdd_v4_emetteur.o
OBJ_TDD4_R = $(OBJDIR)/proto_tdd_v4_recepteur.o

# TDD v%
# -------
tdd%: dirs $(OBJ_COMMON) $(OBJ_APP_NC)
	$(eval export OBJ_APP = $(OBJ_APP_NC))
	$(eval export OBJ_TDD_S = $(OBJ_TDD$*_S))
	$(eval export OBJ_TDD_R = $(OBJ_TDD$*_R))
	make $(SENDER) $(RECEIVER)

# EXEC
# -----
$(SENDER): $(OBJ_COMMON) $(OBJ_APP) $(OBJ_TDD_S)
	$(CC) -o $(SENDER) $(OBJ_COMMON) $(OBJ_APP) $(OBJ_TDD_S) $(LIB)

$(RECEIVER): $(OBJ_COMMON) $(OBJ_APP) $(OBJ_TDD_R)
	$(CC) -o $(RECEIVER) $(OBJ_COMMON) $(OBJ_APP) $(OBJ_TDD_R) $(LIB)

# '%' matches filename
# $@  for the pattern-matched target
# $<  for the pattern-matched dependency
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -o $@ -c $< $(SYS)

dirs:
	@if [ ! -d "./$(OBJDIR)" ]; then mkdir $(OBJDIR); fi
	@if [ ! -d "./$(BINDIR)" ]; then mkdir $(BINDIR); fi

clean:
	rm -f $(OBJDIR)/*.o
	rm -f $(BINDIR)/* $(FILESDIR)/out.*

deliver:
	mkdir $(NOM_ETU)
	cp -r src $(NOM_ETU)
	cp README.txt $(NOM_ETU)
	zip -r $(NOM_ETU).zip $(NOM_ETU)
	rm -rf $(NOM_ETU)
	@echo " "
	@echo "=================================================="
	@echo "Vous devez maintenant déposer l'archive $(NOM_ETU).zip sur Moodle."
	@echo "=================================================="
