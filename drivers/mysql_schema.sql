--
-- Table to hold the individual spamtrap email addresses. This
-- is not to be confused with the "permitted domains".
--
CREATE TABLE IF NOT EXISTS spamtraps(
  `address` VARCHAR(255),
  PRIMARY KEY(`address`)
) engine=innodb, charset=latin1;

--
-- Table to hold the individual permitted domains, which augments
-- the filesystem permitted domains list.
--
CREATE TABLE IF NOT EXISTS domains(
  `domain` VARCHAR(255),
  PRIMARY KEY(`domain`)
) engine=innodb, charset=latin1;

--
-- Table to store the white, grey & trap entries.
--
CREATE TABLE IF NOT EXISTS entries(
  `ip`     VARCHAR(46),
  `helo`   VARCHAR(255),
  `from`   VARCHAR(255),
  `to`     VARCHAR(255),
  `first`  BIGINT,
  `pass`   BIGINT,
  `expire` BIGINT,
  `bcount` INTEGER,
  `pcount` INTEGER,
  `greyd_host` VARCHAR(255),
  PRIMARY KEY (`ip`, `helo`, `from`, `to`),
  INDEX (`greyd_host`)
) engine=innodb, charset=latin1;
