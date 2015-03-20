--
-- Spam trap addresses.
--
CREATE TABLE IF NOT EXISTS spamtraps(
    `address` VARCHAR(1024),
    UNIQUE(`address`)
);

--
-- Grey/white/trap entries.
--
CREATE TABLE IF NOT EXISTS entries(
    `ip`     VARCHAR(46),
    `helo`   VARCHAR(1024),
    `from`   VARCHAR(1024),
    `to`     VARCHAR(1024),
    `first`  UNSIGNED BIGINT,
    `pass`   UNSIGNED BIGINT,
    `expire` UNSIGNED BIGINT,
    `bcount` INTEGER,
    `pcount` INTEGER
);
