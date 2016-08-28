--
-- Table to hold the individual spamtrap email addresses. This
-- is not to be confused with the "permitted domains".
--
CREATE TABLE IF NOT EXISTS spamtraps(
  "address" varchar(255) PRIMARY KEY
);

--
-- Table to hold the individual permitted domains, which augments
-- the filesystem permitted domains list.
--
CREATE TABLE IF NOT EXISTS domains(
  "domain" varchar(255) PRIMARY KEY
);

--
-- Table to store the white, grey & trap entries.
--
CREATE TABLE IF NOT EXISTS entries(
  "ip"         varchar(46),
  "helo"       varchar(255),
  "from"       varchar(255),
  "to"         varchar(255),
  "first"      bigint,
  "pass"       bigint,
  "expire"     bigint,
  "bcount"     integer,
  "pcount"     integer,
  "greyd_host" varchar(255),
  PRIMARY KEY ("ip", "helo", "from", "to")
);

--
-- Index the greyd_host column (B-tree).
--
CREATE INDEX greyd_host_index ON entries("greyd_host");
